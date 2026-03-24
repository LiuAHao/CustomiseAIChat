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
)

type Application struct {
	Config   config.Config
	Store    *memory.Store
	Events   *events.Hub
	Provider llm.Provider
}

func New(cfg config.Config) *Application {
	a := &Application{
		Config:   cfg,
		Store:    memory.NewStore(),
		Events:   events.NewHub(),
		Provider: llm.MockProvider{},
	}
	a.seed()
	return a
}

func (a *Application) seed() {
	now := time.Now().UTC()
	admin := auth.User{
		ID:           shared.NewID("user"),
		Email:        a.Config.SeedAdminEmail,
		Name:         "Platform Admin",
		PasswordHash: shared.HashPassword(a.Config.SeedAdminPassword),
		CreatedAt:    now,
	}
	ws := workspace.Workspace{
		ID:        shared.NewID("ws"),
		Name:      "Default Workspace",
		CreatedBy: admin.ID,
		CreatedAt: now,
	}
	member := workspace.Membership{
		ID:          shared.NewID("member"),
		WorkspaceID: ws.ID,
		UserID:      admin.ID,
		Role:        workspace.RoleOwner,
		CreatedAt:   now,
	}
	a.Store.WithWrite(func() {
		a.Store.Users[admin.ID] = admin
		a.Store.UsersByEmail[strings.ToLower(admin.Email)] = admin.ID
		a.Store.Workspaces[ws.ID] = ws
		a.Store.Memberships[member.ID] = member
	})
}

func (a *Application) Login(email, password string) (map[string]any, error) {
	var user auth.User
	a.Store.WithRead(func() {
		if id, ok := a.Store.UsersByEmail[strings.ToLower(email)]; ok {
			user = a.Store.Users[id]
		}
	})
	if user.ID == "" || user.PasswordHash != shared.HashPassword(password) {
		return nil, errors.New("invalid credentials")
	}
	token := auth.SessionToken{
		Token:     shared.NewID("token"),
		UserID:    user.ID,
		ExpiresAt: time.Now().Add(24 * time.Hour),
	}
	a.Store.WithWrite(func() { a.Store.Tokens[token.Token] = token })
	a.appendAudit("", user.ID, "auth.login", "user", user.ID, nil)
	return map[string]any{"token": token.Token, "user": user}, nil
}

func (a *Application) Authenticate(token string) (auth.User, error) {
	var sessionToken auth.SessionToken
	a.Store.WithRead(func() { sessionToken = a.Store.Tokens[token] })
	if sessionToken.Token == "" || sessionToken.ExpiresAt.Before(time.Now()) {
		return auth.User{}, errors.New("unauthorized")
	}
	var user auth.User
	a.Store.WithRead(func() { user = a.Store.Users[sessionToken.UserID] })
	if user.ID == "" {
		return auth.User{}, errors.New("unauthorized")
	}
	return user, nil
}

func (a *Application) ListWorkspaces(userID string) []workspace.Workspace {
	items := make([]workspace.Workspace, 0)
	a.Store.WithRead(func() {
		for _, membership := range a.Store.Memberships {
			if membership.UserID == userID {
				items = append(items, a.Store.Workspaces[membership.WorkspaceID])
			}
		}
	})
	return items
}

func (a *Application) CreateWorkspace(userID, name string) workspace.Workspace {
	item := workspace.Workspace{
		ID:        shared.NewID("ws"),
		Name:      name,
		CreatedBy: userID,
		CreatedAt: time.Now().UTC(),
	}
	member := workspace.Membership{
		ID:          shared.NewID("member"),
		WorkspaceID: item.ID,
		UserID:      userID,
		Role:        workspace.RoleOwner,
		CreatedAt:   time.Now().UTC(),
	}
	a.Store.WithWrite(func() {
		a.Store.Workspaces[item.ID] = item
		a.Store.Memberships[member.ID] = member
	})
	a.appendAudit(item.ID, userID, "workspace.create", "workspace", item.ID, map[string]any{"name": name})
	return item
}

func (a *Application) CreateAgent(userID string, req CreateAgentRequest) (agent.Agent, error) {
	if !a.userInWorkspace(userID, req.WorkspaceID) {
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
	a.Store.WithWrite(func() { a.Store.Agents[item.ID] = item })
	a.appendAudit(req.WorkspaceID, userID, "agent.create", "agent", item.ID, map[string]any{"name": item.Name})
	return item, nil
}

func (a *Application) ListAgents(workspaceID string) []agent.Agent {
	items := make([]agent.Agent, 0)
	a.Store.WithRead(func() {
		for _, item := range a.Store.Agents {
			if item.WorkspaceID == workspaceID {
				items = append(items, item)
			}
		}
	})
	return items
}

func (a *Application) CreateVersion(userID, agentID string) (agent.Version, error) {
	var current agent.Agent
	count := 0
	a.Store.WithRead(func() {
		current = a.Store.Agents[agentID]
		for _, version := range a.Store.AgentVersions {
			if version.AgentID == agentID {
				count++
			}
		}
	})
	if current.ID == "" {
		return agent.Version{}, errors.New("agent not found")
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
	a.Store.WithWrite(func() { a.Store.AgentVersions[version.ID] = version })
	a.appendAudit(current.WorkspaceID, userID, "agent.version.create", "agent_version", version.ID, map[string]any{"agent_id": agentID})
	return version, nil
}

func (a *Application) PublishVersion(userID, agentID, versionID string) error {
	var current agent.Agent
	var version agent.Version
	a.Store.WithRead(func() {
		current = a.Store.Agents[agentID]
		version = a.Store.AgentVersions[versionID]
	})
	if current.ID == "" || version.ID == "" || version.AgentID != agentID {
		return errors.New("agent or version not found")
	}
	current.PublishedVerID = versionID
	current.UpdatedAt = time.Now().UTC()
	a.Store.WithWrite(func() { a.Store.Agents[agentID] = current })
	a.appendAudit(current.WorkspaceID, userID, "agent.publish", "agent", agentID, map[string]any{"version_id": versionID})
	return nil
}

func (a *Application) CreateTool(userID string, req CreateToolRequest) (tool.Tool, error) {
	if !a.userInWorkspace(userID, req.WorkspaceID) {
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
	a.Store.WithWrite(func() { a.Store.Tools[item.ID] = item })
	a.appendAudit(req.WorkspaceID, userID, "tool.create", "tool", item.ID, map[string]any{"name": item.Name})
	return item, nil
}

func (a *Application) ListTools(workspaceID string) []tool.Tool {
	items := make([]tool.Tool, 0)
	a.Store.WithRead(func() {
		for _, item := range a.Store.Tools {
			if item.WorkspaceID == workspaceID {
				items = append(items, item)
			}
		}
	})
	return items
}

func (a *Application) CreateSession(userID string, req CreateSessionRequest) (session.Session, error) {
	if !a.userInWorkspace(userID, req.WorkspaceID) {
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
	a.Store.WithWrite(func() { a.Store.Sessions[item.ID] = item })
	a.appendAudit(req.WorkspaceID, userID, "session.create", "session", item.ID, nil)
	return item, nil
}

func (a *Application) ListMessages(sessionID string) []session.Message {
	var items []session.Message
	a.Store.WithRead(func() { items = append(items, a.Store.Messages[sessionID]...) })
	return items
}

func (a *Application) CreateSchedule(userID string, req CreateScheduleRequest) (task.Schedule, error) {
	if !a.userInWorkspace(userID, req.WorkspaceID) {
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
	a.Store.WithWrite(func() { a.Store.Schedules[item.ID] = item })
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
	due := make([]task.Schedule, 0)
	a.Store.WithRead(func() {
		for _, schedule := range a.Store.Schedules {
			if !schedule.NextRunAt.After(now) {
				due = append(due, schedule)
			}
		}
	})
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
		a.Store.WithWrite(func() { a.Store.Schedules[schedule.ID] = schedule })
	}
}

func (a *Application) ExecuteTask(userID string, req ExecuteTaskRequest) (task.Task, error) {
	if !a.userInWorkspace(userID, req.WorkspaceID) {
		return task.Task{}, errors.New("workspace access denied")
	}
	agentRecord, ok := a.getAgent(req.AgentID)
	if !ok {
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
	a.Store.WithWrite(func() {
		a.Store.Tasks[job.ID] = job
		a.Store.Messages[sessionID] = append(a.Store.Messages[sessionID], session.Message{
			ID:        shared.NewID("msg"),
			SessionID: sessionID,
			Role:      session.RoleUser,
			Content:   req.Prompt,
			TraceID:   job.TraceID,
			CreatedAt: now,
		})
	})
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
	a.Store.WithWrite(func() { a.Store.Tasks[job.ID] = job })
	a.publishEvent("task.started", job.ID, job.TraceID, map[string]any{"task_id": job.ID})

	steps := make([]task.Step, 0)
	seq := 1
	for _, name := range a.Provider.Plan(job.Prompt) {
		steps = append(steps, task.Step{
			ID:        shared.NewID("step"),
			TaskID:    job.ID,
			Name:      name,
			Status:    task.StepCompleted,
			Output:    "planner completed",
			Sequence:  seq,
			CreatedAt: time.Now().UTC(),
			UpdatedAt: time.Now().UTC(),
		})
		a.publishEvent("task.step.completed", job.ID, job.TraceID, map[string]any{"task_id": job.ID, "step_name": name})
		seq++
	}

	toolOutputs := make([]string, 0, len(job.ToolCalls))
	for _, call := range job.ToolCalls {
		t, ok := a.getTool(call.ToolID)
		if !ok || !t.Enabled {
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
			a.Store.WithWrite(func() {
				a.Store.Approvals[appr.ID] = appr
				a.Store.Tasks[job.ID] = job
				a.Store.TaskSteps[job.ID] = append(append(a.Store.TaskSteps[job.ID], steps...), step)
			})
			a.appendAudit(job.WorkspaceID, job.CreatedBy, "approval.create", "approval", appr.ID, map[string]any{"tool_id": t.ID, "task_id": job.ID})
			a.publishEvent("task.waiting_approval", job.ID, job.TraceID, map[string]any{"task_id": job.ID, "approval_id": appr.ID, "reason": appr.Reason})
			return
		}
		output, outputJSON := a.executeTool(t, call.Input)
		step.Status = task.StepCompleted
		step.Output = output
		toolOutputs = append(toolOutputs, output)
		a.Store.WithWrite(func() { a.Store.TaskSteps[job.ID] = append(a.Store.TaskSteps[job.ID], step) })
		a.publishEvent("task.stream.delta", job.ID, job.TraceID, map[string]any{"task_id": job.ID, "delta": outputJSON})
	}

	result := a.Provider.Complete(job.Prompt, toolOutputs)
	job.Status = task.StatusCompleted
	job.Result = result
	job.UpdatedAt = time.Now().UTC()
	a.Store.WithWrite(func() {
		a.Store.TaskSteps[job.ID] = append(a.Store.TaskSteps[job.ID], steps...)
		a.Store.Tasks[job.ID] = job
		a.Store.Messages[job.SessionID] = append(a.Store.Messages[job.SessionID], session.Message{
			ID:        shared.NewID("msg"),
			SessionID: job.SessionID,
			Role:      session.RoleAssistant,
			Content:   result,
			TraceID:   job.TraceID,
			CreatedAt: time.Now().UTC(),
		})
	})
	a.publishEvent("task.completed", job.ID, job.TraceID, map[string]any{"task_id": job.ID, "result": result})
}

func (a *Application) Approve(userID, approvalID string, approved bool) (approval.Approval, error) {
	var item approval.Approval
	a.Store.WithRead(func() { item = a.Store.Approvals[approvalID] })
	if item.ID == "" {
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
	a.Store.WithWrite(func() { a.Store.Approvals[item.ID] = item })
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
	var steps []task.Step
	a.Store.WithRead(func() { steps = append(steps, a.Store.TaskSteps[job.ID]...) })
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
	a.Store.WithWrite(func() {
		a.Store.TaskSteps[job.ID] = steps
		a.Store.Tasks[job.ID] = job
		a.Store.Messages[job.SessionID] = append(a.Store.Messages[job.SessionID], session.Message{
			ID:        shared.NewID("msg"),
			SessionID: job.SessionID,
			Role:      session.RoleAssistant,
			Content:   job.Result,
			TraceID:   job.TraceID,
			CreatedAt: time.Now().UTC(),
		})
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
	a.Store.WithWrite(func() { a.Store.Tasks[job.ID] = job })
	a.appendAudit(job.WorkspaceID, userID, "task.cancel", "task", taskID, nil)
	return job, nil
}

func (a *Application) GetTask(taskID string) (task.Task, error) {
	var item task.Task
	a.Store.WithRead(func() { item = a.Store.Tasks[taskID] })
	if item.ID == "" {
		return task.Task{}, errors.New("task not found")
	}
	return item, nil
}

func (a *Application) GetTaskSteps(taskID string) []task.Step {
	var steps []task.Step
	a.Store.WithRead(func() { steps = append(steps, a.Store.TaskSteps[taskID]...) })
	return steps
}

func (a *Application) ListApprovals(workspaceID string) []approval.Approval {
	items := make([]approval.Approval, 0)
	a.Store.WithRead(func() {
		for _, item := range a.Store.Approvals {
			if workspaceID == "" || item.WorkspaceID == workspaceID {
				items = append(items, item)
			}
		}
	})
	return items
}

func (a *Application) ListAuditEvents(workspaceID string) []audit.Event {
	items := make([]audit.Event, 0)
	a.Store.WithRead(func() {
		for _, item := range a.Store.AuditEvents {
			if workspaceID == "" || item.WorkspaceID == workspaceID {
				items = append(items, item)
			}
		}
	})
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
	a.Store.WithWrite(func() { a.Store.Tasks[job.ID] = job })
	a.publishEvent("task.failed", job.ID, job.TraceID, map[string]any{"task_id": job.ID, "error": reason})
}

func (a *Application) userInWorkspace(userID, workspaceID string) bool {
	found := false
	a.Store.WithRead(func() {
		for _, membership := range a.Store.Memberships {
			if membership.UserID == userID && membership.WorkspaceID == workspaceID {
				found = true
				return
			}
		}
	})
	return found
}

func (a *Application) getAgent(agentID string) (agent.Agent, bool) {
	var item agent.Agent
	a.Store.WithRead(func() { item = a.Store.Agents[agentID] })
	return item, item.ID != ""
}

func (a *Application) getTool(toolID string) (tool.Tool, bool) {
	var item tool.Tool
	a.Store.WithRead(func() { item = a.Store.Tools[toolID] })
	return item, item.ID != ""
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
	a.Store.WithWrite(func() { a.Store.AuditEvents = append(a.Store.AuditEvents, entry) })
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
	WorkspaceID string          `json:"workspace_id"`
	AgentID     string          `json:"agent_id"`
	SessionID   string          `json:"session_id"`
	SessionTitle string         `json:"session_title"`
	Prompt      string          `json:"prompt"`
	Async       bool            `json:"async"`
	ToolCalls   []tool.CallSpec `json:"tool_calls"`
}

type CreateScheduleRequest struct {
	WorkspaceID string `json:"workspace_id"`
	AgentID     string `json:"agent_id"`
	Name        string `json:"name"`
	Prompt      string `json:"prompt"`
	Cron        string `json:"cron"`
}
