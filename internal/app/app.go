package app

import (
	"encoding/json"
	"errors"
	"fmt"
	"strings"
	"time"

	"go-agent-platform/internal/config"
	"go-agent-platform/internal/domain/agent"
	"go-agent-platform/internal/domain/approval"
	"go-agent-platform/internal/domain/audit"
	"go-agent-platform/internal/domain/auth"
	"go-agent-platform/internal/domain/session"
	"go-agent-platform/internal/domain/shared"
	"go-agent-platform/internal/domain/task"
	"go-agent-platform/internal/domain/tool"
	"go-agent-platform/internal/domain/workspace"
	"go-agent-platform/internal/platform/events"
	"go-agent-platform/internal/platform/llm"
	"go-agent-platform/internal/platform/memory"
	"go-agent-platform/internal/platform/postgres"
)

type Application struct {
	Config   config.Config
	Store    Store
	Events   *events.Hub
	Provider llm.Provider
}

func New(cfg config.Config) (*Application, error) {
	store, err := newStore(cfg)
	if err != nil {
		return nil, err
	}
	if err := store.EnsureSeedData(cfg); err != nil {
		_ = store.Close()
		return nil, err
	}
	return &Application{
		Config:   cfg,
		Store:    store,
		Events:   events.NewHub(),
		Provider: llm.MockProvider{},
	}, nil
}

func newStore(cfg config.Config) (Store, error) {
	switch strings.ToLower(strings.TrimSpace(cfg.StorageDriver)) {
	case "", "memory":
		return memory.NewStore(), nil
	case "postgres":
		return postgres.New(cfg)
	default:
		return nil, fmt.Errorf("unsupported storage driver: %s", cfg.StorageDriver)
	}
}

func (a *Application) Close() error {
	if a.Store == nil {
		return nil
	}
	return a.Store.Close()
}

func (a *Application) Login(email, password string) (map[string]any, error) {
	user, err := a.Store.FindUserByEmail(email)
	if err != nil || user.PasswordHash != shared.HashPassword(password) {
		return nil, errors.New("invalid credentials")
	}

	token := auth.SessionToken{
		Token:     shared.NewID("token"),
		UserID:    user.ID,
		ExpiresAt: time.Now().Add(24 * time.Hour),
	}
	if err := a.Store.SaveSessionToken(token); err != nil {
		return nil, err
	}
	a.appendAudit("", user.ID, "auth.login", "user", user.ID, nil)
	return map[string]any{"token": token.Token, "user": user}, nil
}

func (a *Application) Authenticate(token string) (auth.User, error) {
	sessionToken, err := a.Store.FindSessionToken(token)
	if err != nil || sessionToken.ExpiresAt.Before(time.Now()) {
		return auth.User{}, errors.New("unauthorized")
	}
	user, err := a.Store.FindUserByID(sessionToken.UserID)
	if err != nil {
		return auth.User{}, errors.New("unauthorized")
	}
	return user, nil
}

func (a *Application) ListWorkspaces(userID string) []workspace.Workspace {
	items, err := a.Store.ListWorkspacesByUser(userID)
	if err != nil {
		return []workspace.Workspace{}
	}
	return items
}

func (a *Application) CreateWorkspace(userID, name string) workspace.Workspace {
	now := time.Now().UTC()
	item := workspace.Workspace{
		ID:        shared.NewID("ws"),
		Name:      name,
		CreatedBy: userID,
		CreatedAt: now,
	}
	member := workspace.Membership{
		ID:          shared.NewID("member"),
		WorkspaceID: item.ID,
		UserID:      userID,
		Role:        workspace.RoleOwner,
		CreatedAt:   now,
	}
	_ = a.Store.CreateWorkspace(item, member)
	a.appendAudit(item.ID, userID, "workspace.create", "workspace", item.ID, map[string]any{"name": name})
	return item
}

func (a *Application) CreateAgent(userID string, req CreateAgentRequest) (agent.Agent, error) {
	if ok, err := a.userInWorkspace(userID, req.WorkspaceID); err != nil || !ok {
		return agent.Agent{}, errors.New("workspace access denied")
	}
	now := time.Now().UTC()
	item := agent.Agent{
		ID:            shared.NewID("agent"),
		WorkspaceID:   req.WorkspaceID,
		Name:          req.Name,
		Description:   req.Description,
		SystemPrompt:  req.SystemPrompt,
		Model:         req.Model,
		ToolPolicy:    req.ToolPolicy,
		RuntimePolicy: req.RuntimePolicy,
		CreatedBy:     userID,
		CreatedAt:     now,
		UpdatedAt:     now,
	}
	if err := a.Store.SaveAgent(item); err != nil {
		return agent.Agent{}, err
	}
	a.appendAudit(req.WorkspaceID, userID, "agent.create", "agent", item.ID, map[string]any{"name": item.Name})
	return item, nil
}

func (a *Application) ListAgents(workspaceID string) []agent.Agent {
	items, err := a.Store.ListAgents(workspaceID)
	if err != nil {
		return []agent.Agent{}
	}
	return items
}

func (a *Application) CreateVersion(userID, agentID string) (agent.Version, error) {
	current, err := a.Store.FindAgentByID(agentID)
	if err != nil {
		return agent.Version{}, errors.New("agent not found")
	}
	count, err := a.Store.CountAgentVersions(agentID)
	if err != nil {
		return agent.Version{}, err
	}
	snapshot := map[string]any{
		"name":           current.Name,
		"description":    current.Description,
		"system_prompt":  current.SystemPrompt,
		"model":          current.Model,
		"tool_policy":    current.ToolPolicy,
		"runtime_policy": current.RuntimePolicy,
	}
	version := agent.Version{
		ID:            shared.NewID("agentver"),
		AgentID:       agentID,
		VersionNumber: count + 1,
		Snapshot:      snapshot,
		CreatedBy:     userID,
		CreatedAt:     time.Now().UTC(),
	}
	if err := a.Store.SaveAgentVersion(version); err != nil {
		return agent.Version{}, err
	}
	a.appendAudit(current.WorkspaceID, userID, "agent.version.create", "agent_version", version.ID, map[string]any{"agent_id": agentID})
	return version, nil
}

func (a *Application) PublishVersion(userID, agentID, versionID string) error {
	current, err := a.Store.FindAgentByID(agentID)
	if err != nil {
		return errors.New("agent or version not found")
	}
	version, err := a.Store.FindAgentVersion(versionID)
	if err != nil || version.AgentID != agentID {
		return errors.New("agent or version not found")
	}
	current.PublishedVerID = versionID
	current.UpdatedAt = time.Now().UTC()
	if err := a.Store.UpdateAgent(current); err != nil {
		return err
	}
	a.appendAudit(current.WorkspaceID, userID, "agent.publish", "agent", agentID, map[string]any{"version_id": versionID})
	return nil
}

func (a *Application) CreateTool(userID string, req CreateToolRequest) (tool.Tool, error) {
	if ok, err := a.userInWorkspace(userID, req.WorkspaceID); err != nil || !ok {
		return tool.Tool{}, errors.New("workspace access denied")
	}
	item := tool.Tool{
		ID:               shared.NewID("tool"),
		WorkspaceID:      req.WorkspaceID,
		Name:             req.Name,
		Description:      req.Description,
		Schema:           req.Schema,
		Config:           req.Config,
		RequiresApproval: req.RequiresApproval,
		Enabled:          true,
		CreatedBy:        userID,
		CreatedAt:        time.Now().UTC(),
	}
	if err := a.Store.SaveTool(item); err != nil {
		return tool.Tool{}, err
	}
	a.appendAudit(req.WorkspaceID, userID, "tool.create", "tool", item.ID, map[string]any{"name": item.Name})
	return item, nil
}

func (a *Application) ListTools(workspaceID string) []tool.Tool {
	items, err := a.Store.ListTools(workspaceID)
	if err != nil {
		return []tool.Tool{}
	}
	return items
}

func (a *Application) CreateSession(userID string, req CreateSessionRequest) (session.Session, error) {
	if ok, err := a.userInWorkspace(userID, req.WorkspaceID); err != nil || !ok {
		return session.Session{}, errors.New("workspace access denied")
	}
	item := session.Session{
		ID:          shared.NewID("session"),
		WorkspaceID: req.WorkspaceID,
		AgentID:     req.AgentID,
		CreatedBy:   userID,
		Title:       req.Title,
		CreatedAt:   time.Now().UTC(),
		UpdatedAt:   time.Now().UTC(),
	}
	if err := a.Store.SaveSession(item); err != nil {
		return session.Session{}, err
	}
	a.appendAudit(req.WorkspaceID, userID, "session.create", "session", item.ID, nil)
	return item, nil
}

func (a *Application) ListMessages(sessionID string) []session.Message {
	items, err := a.Store.ListMessages(sessionID)
	if err != nil {
		return []session.Message{}
	}
	return items
}

func (a *Application) CreateSchedule(userID string, req CreateScheduleRequest) (task.Schedule, error) {
	if ok, err := a.userInWorkspace(userID, req.WorkspaceID); err != nil || !ok {
		return task.Schedule{}, errors.New("workspace access denied")
	}
	nextRunAt, interval, err := parseSchedule(req.Cron)
	if err != nil {
		return task.Schedule{}, err
	}
	item := task.Schedule{
		ID:          shared.NewID("schedule"),
		WorkspaceID: req.WorkspaceID,
		AgentID:     req.AgentID,
		Name:        req.Name,
		Prompt:      req.Prompt,
		Cron:        req.Cron,
		Interval:    interval.String(),
		NextRunAt:   nextRunAt,
		CreatedBy:   userID,
		CreatedAt:   time.Now().UTC(),
	}
	if err := a.Store.SaveSchedule(item); err != nil {
		return task.Schedule{}, err
	}
	a.appendAudit(req.WorkspaceID, userID, "schedule.create", "schedule", item.ID, map[string]any{"cron": req.Cron})
	return item, nil
}

func parseSchedule(expr string) (time.Time, time.Duration, error) {
	if !strings.HasPrefix(expr, "@every ") {
		return time.Time{}, 0, errors.New("only '@every <duration>' schedules are supported in v1")
	}
	d, err := time.ParseDuration(strings.TrimPrefix(expr, "@every "))
	if err != nil || d <= 0 {
		return time.Time{}, 0, errors.New("invalid duration")
	}
	return time.Now().UTC().Add(d), d, nil
}

func (a *Application) RunDueSchedules() {
	now := time.Now().UTC()
	due, err := a.Store.ListDueSchedules(now)
	if err != nil {
		return
	}
	for _, schedule := range due {
		_, _ = a.ExecuteTask(schedule.CreatedBy, ExecuteTaskRequest{
			WorkspaceID: schedule.WorkspaceID,
			AgentID:     schedule.AgentID,
			Prompt:      schedule.Prompt,
			Async:       true,
		})
		dur, err := time.ParseDuration(schedule.Interval)
		if err != nil {
			continue
		}
		schedule.NextRunAt = time.Now().UTC().Add(dur)
		_ = a.Store.UpdateSchedule(schedule)
	}
}

func (a *Application) ExecuteTask(userID string, req ExecuteTaskRequest) (task.Task, error) {
	if ok, err := a.userInWorkspace(userID, req.WorkspaceID); err != nil || !ok {
		return task.Task{}, errors.New("workspace access denied")
	}
	agentRecord, err := a.Store.FindAgentByID(req.AgentID)
	if err != nil {
		return task.Task{}, errors.New("agent not found")
	}
	if agentRecord.PublishedVerID == "" {
		return task.Task{}, errors.New("agent has no published version")
	}

	now := time.Now().UTC()
	sessionID := req.SessionID
	if sessionID == "" {
		created, err := a.CreateSession(userID, CreateSessionRequest{
			WorkspaceID: req.WorkspaceID,
			AgentID:     req.AgentID,
			Title:       firstNonEmpty(req.SessionTitle, req.Prompt),
		})
		if err != nil {
			return task.Task{}, err
		}
		sessionID = created.ID
	}

	job := task.Task{
		ID:          shared.NewID("task"),
		TraceID:     shared.NewID("trace"),
		WorkspaceID: req.WorkspaceID,
		AgentID:     req.AgentID,
		SessionID:   sessionID,
		Prompt:      req.Prompt,
		Status:      task.StatusPending,
		CreatedBy:   userID,
		ToolCalls:   req.ToolCalls,
		CreatedAt:   now,
		UpdatedAt:   now,
	}
	if err := a.Store.SaveTask(job); err != nil {
		return task.Task{}, err
	}
	if err := a.Store.SaveMessage(session.Message{
		ID:        shared.NewID("msg"),
		SessionID: sessionID,
		Role:      session.RoleUser,
		Content:   req.Prompt,
		TraceID:   job.TraceID,
		CreatedAt: now,
	}); err != nil {
		return task.Task{}, err
	}

	a.appendAudit(req.WorkspaceID, userID, "task.create", "task", job.ID, map[string]any{"agent_id": req.AgentID})
	if req.Async {
		go a.runTask(job.ID)
		return job, nil
	}
	a.runTask(job.ID)
	return a.GetTask(job.ID)
}

func (a *Application) runTask(taskID string) {
	job, err := a.GetTask(taskID)
	if err != nil {
		return
	}
	job.Status = task.StatusRunning
	job.UpdatedAt = time.Now().UTC()
	_ = a.Store.UpdateTask(job)
	a.publishEvent("task.started", job.ID, job.TraceID, map[string]any{"task_id": job.ID})

	seq := 1
	for _, name := range a.Provider.Plan(job.Prompt) {
		step := task.Step{
			ID:        shared.NewID("step"),
			TaskID:    job.ID,
			Name:      name,
			Status:    task.StepCompleted,
			Output:    "planner completed",
			Sequence:  seq,
			CreatedAt: time.Now().UTC(),
			UpdatedAt: time.Now().UTC(),
		}
		_ = a.Store.SaveTaskStep(step)
		a.publishEvent("task.step.completed", job.ID, job.TraceID, map[string]any{"task_id": job.ID, "step_name": name})
		seq++
	}

	toolOutputs := make([]string, 0, len(job.ToolCalls))
	for _, call := range job.ToolCalls {
		t, err := a.Store.FindToolByID(call.ToolID)
		if err != nil || !t.Enabled {
			a.failTask(job.ID, fmt.Sprintf("tool %s not available", call.ToolID))
			return
		}
		step := task.Step{
			ID:        shared.NewID("step"),
			TaskID:    job.ID,
			Name:      "tool:" + t.Name,
			Status:    task.StepRunning,
			Sequence:  seq,
			CreatedAt: time.Now().UTC(),
			UpdatedAt: time.Now().UTC(),
		}
		seq++
		a.publishEvent("task.step.started", job.ID, job.TraceID, map[string]any{"task_id": job.ID, "step_name": step.Name})

		if t.RequiresApproval {
			step.Status = task.StepWaiting
			appr := approval.Approval{
				ID:          shared.NewID("approval"),
				WorkspaceID: job.WorkspaceID,
				TaskID:      job.ID,
				StepID:      step.ID,
				Reason:      "high risk tool requires approval: " + t.Name,
				Status:      approval.StatusPending,
				CreatedAt:   time.Now().UTC(),
			}
			step.ApprovalRef = appr.ID
			job.Status = task.StatusWaitingApproval
			job.ApprovalID = appr.ID
			job.UpdatedAt = time.Now().UTC()

			_ = a.Store.SaveTaskStep(step)
			_ = a.Store.SaveApproval(appr)
			_ = a.Store.UpdateTask(job)

			a.appendAudit(job.WorkspaceID, job.CreatedBy, "approval.create", "approval", appr.ID, map[string]any{"tool_id": t.ID, "task_id": job.ID})
			a.publishEvent("task.waiting_approval", job.ID, job.TraceID, map[string]any{"task_id": job.ID, "approval_id": appr.ID, "reason": appr.Reason})
			return
		}

		output, outputJSON := a.executeTool(t, call.Input)
		step.Status = task.StepCompleted
		step.Output = output
		step.UpdatedAt = time.Now().UTC()
		toolOutputs = append(toolOutputs, output)
		_ = a.Store.SaveTaskStep(step)
		a.publishEvent("task.stream.delta", job.ID, job.TraceID, map[string]any{"task_id": job.ID, "delta": outputJSON})
	}

	result := a.Provider.Complete(job.Prompt, toolOutputs)
	job.Status = task.StatusCompleted
	job.Result = result
	job.UpdatedAt = time.Now().UTC()
	_ = a.Store.UpdateTask(job)
	_ = a.Store.SaveMessage(session.Message{
		ID:        shared.NewID("msg"),
		SessionID: job.SessionID,
		Role:      session.RoleAssistant,
		Content:   result,
		TraceID:   job.TraceID,
		CreatedAt: time.Now().UTC(),
	})
	a.publishEvent("task.completed", job.ID, job.TraceID, map[string]any{"task_id": job.ID, "result": result})
}

func (a *Application) Approve(userID, approvalID string, approved bool) (approval.Approval, error) {
	item, err := a.Store.FindApprovalByID(approvalID)
	if err != nil {
		return approval.Approval{}, errors.New("approval not found")
	}
	if item.Status != approval.StatusPending {
		return approval.Approval{}, errors.New("approval already decided")
	}
	if approved {
		item.Status = approval.StatusApproved
	} else {
		item.Status = approval.StatusRejected
	}
	item.DecisionBy = userID
	item.DecisionAt = time.Now().UTC()
	if err := a.Store.UpdateApproval(item); err != nil {
		return approval.Approval{}, err
	}
	if approved {
		a.resumeApprovedTask(item.TaskID, item.StepID)
	} else {
		a.failTask(item.TaskID, "approval rejected")
	}
	a.appendAudit(item.WorkspaceID, userID, "approval.decide", "approval", item.ID, map[string]any{"approved": approved})
	return item, nil
}

func (a *Application) resumeApprovedTask(taskID, stepID string) {
	job, err := a.GetTask(taskID)
	if err != nil {
		return
	}
	steps, err := a.Store.ListTaskSteps(job.ID)
	if err != nil {
		return
	}
	for idx := range steps {
		if steps[idx].ID == stepID {
			steps[idx].Status = task.StepCompleted
			steps[idx].Output = "approval granted and tool executed"
			steps[idx].UpdatedAt = time.Now().UTC()
		}
	}
	job.Status = task.StatusCompleted
	job.Result = "task resumed after approval"
	job.UpdatedAt = time.Now().UTC()
	_ = a.Store.ReplaceTaskSteps(job.ID, steps)
	_ = a.Store.UpdateTask(job)
	_ = a.Store.SaveMessage(session.Message{
		ID:        shared.NewID("msg"),
		SessionID: job.SessionID,
		Role:      session.RoleAssistant,
		Content:   job.Result,
		TraceID:   job.TraceID,
		CreatedAt: time.Now().UTC(),
	})
	a.publishEvent("task.completed", job.ID, job.TraceID, map[string]any{"task_id": job.ID, "result": job.Result})
}

func (a *Application) CancelTask(userID, taskID string) (task.Task, error) {
	job, err := a.GetTask(taskID)
	if err != nil {
		return task.Task{}, err
	}
	job.Status = task.StatusCanceled
	job.UpdatedAt = time.Now().UTC()
	if err := a.Store.UpdateTask(job); err != nil {
		return task.Task{}, err
	}
	a.appendAudit(job.WorkspaceID, userID, "task.cancel", "task", taskID, nil)
	return job, nil
}

func (a *Application) GetTask(taskID string) (task.Task, error) {
	item, err := a.Store.FindTaskByID(taskID)
	if err != nil {
		return task.Task{}, errors.New("task not found")
	}
	return item, nil
}

func (a *Application) GetTaskSteps(taskID string) []task.Step {
	items, err := a.Store.ListTaskSteps(taskID)
	if err != nil {
		return []task.Step{}
	}
	return items
}

func (a *Application) ListApprovals(workspaceID string) []approval.Approval {
	items, err := a.Store.ListApprovals(workspaceID)
	if err != nil {
		return []approval.Approval{}
	}
	return items
}

func (a *Application) ListAuditEvents(workspaceID string) []audit.Event {
	items, err := a.Store.ListAuditEvents(workspaceID)
	if err != nil {
		return []audit.Event{}
	}
	return items
}

func (a *Application) executeTool(t tool.Tool, input map[string]any) (string, string) {
	payload, _ := json.Marshal(input)
	result := fmt.Sprintf("executed tool %s with input %s", t.Name, string(payload))
	return result, result
}

func (a *Application) failTask(taskID, reason string) {
	job, err := a.GetTask(taskID)
	if err != nil {
		return
	}
	job.Status = task.StatusFailed
	job.Error = reason
	job.UpdatedAt = time.Now().UTC()
	_ = a.Store.UpdateTask(job)
	a.publishEvent("task.failed", job.ID, job.TraceID, map[string]any{"task_id": job.ID, "error": reason})
}

func (a *Application) userInWorkspace(userID, workspaceID string) (bool, error) {
	return a.Store.UserInWorkspace(userID, workspaceID)
}

func (a *Application) appendAudit(workspaceID, actorID, action, resource, resourceID string, metadata map[string]any) {
	entry := audit.Event{
		ID:          shared.NewID("audit"),
		WorkspaceID: workspaceID,
		ActorID:     actorID,
		Action:      action,
		Resource:    resource,
		ResourceID:  resourceID,
		Metadata:    metadata,
		CreatedAt:   time.Now().UTC(),
	}
	_ = a.Store.SaveAuditEvent(entry)
}

func (a *Application) publishEvent(eventType, taskID, traceID string, payload map[string]any) {
	packet := map[string]any{
		"type":      eventType,
		"task_id":   taskID,
		"trace_id":  traceID,
		"payload":   payload,
		"timestamp": time.Now().UTC(),
	}
	if raw, err := json.Marshal(packet); err == nil {
		a.Events.Publish(raw)
	}
}

func firstNonEmpty(items ...string) string {
	for _, item := range items {
		if strings.TrimSpace(item) != "" {
			return item
		}
	}
	return "Untitled Session"
}

type CreateAgentRequest struct {
	WorkspaceID   string   `json:"workspace_id"`
	Name          string   `json:"name"`
	Description   string   `json:"description"`
	SystemPrompt  string   `json:"system_prompt"`
	Model         string   `json:"model"`
	ToolPolicy    []string `json:"tool_policy"`
	RuntimePolicy string   `json:"runtime_policy"`
}

type CreateToolRequest struct {
	WorkspaceID      string         `json:"workspace_id"`
	Name             string         `json:"name"`
	Description      string         `json:"description"`
	Schema           map[string]any `json:"schema"`
	Config           map[string]any `json:"config"`
	RequiresApproval bool           `json:"requires_approval"`
}

type CreateSessionRequest struct {
	WorkspaceID string `json:"workspace_id"`
	AgentID     string `json:"agent_id"`
	Title       string `json:"title"`
}

type ExecuteTaskRequest struct {
	WorkspaceID  string          `json:"workspace_id"`
	AgentID      string          `json:"agent_id"`
	SessionID    string          `json:"session_id"`
	SessionTitle string          `json:"session_title"`
	Prompt       string          `json:"prompt"`
	Async        bool            `json:"async"`
	ToolCalls    []tool.CallSpec `json:"tool_calls"`
}

type CreateScheduleRequest struct {
	WorkspaceID string `json:"workspace_id"`
	AgentID     string `json:"agent_id"`
	Name        string `json:"name"`
	Prompt      string `json:"prompt"`
	Cron        string `json:"cron"`
}
