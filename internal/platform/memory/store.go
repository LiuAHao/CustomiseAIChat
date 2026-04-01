package memory

import (
	"errors"
	"strings"
	"sync"
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
)

type Store struct {
	mu sync.RWMutex

	users         map[string]auth.User
	usersByEmail  map[string]string
	tokens        map[string]auth.SessionToken
	workspaces    map[string]workspace.Workspace
	memberships   map[string]workspace.Membership
	agents        map[string]agent.Agent
	agentVersions map[string]agent.Version
	tools         map[string]tool.Tool
	sessions      map[string]session.Session
	messages      map[string][]session.Message
	tasks         map[string]task.Task
	taskSteps     map[string][]task.Step
	approvals     map[string]approval.Approval
	auditEvents   []audit.Event
	schedules     map[string]task.Schedule
}

func NewStore() *Store {
	return &Store{
		users:         make(map[string]auth.User),
		usersByEmail:  make(map[string]string),
		tokens:        make(map[string]auth.SessionToken),
		workspaces:    make(map[string]workspace.Workspace),
		memberships:   make(map[string]workspace.Membership),
		agents:        make(map[string]agent.Agent),
		agentVersions: make(map[string]agent.Version),
		tools:         make(map[string]tool.Tool),
		sessions:      make(map[string]session.Session),
		messages:      make(map[string][]session.Message),
		tasks:         make(map[string]task.Task),
		taskSteps:     make(map[string][]task.Step),
		approvals:     make(map[string]approval.Approval),
		auditEvents:   make([]audit.Event, 0),
		schedules:     make(map[string]task.Schedule),
	}
}

func (s *Store) Close() error {
	return nil
}

func (s *Store) EnsureSeedData(cfg config.Config) error {
	s.mu.Lock()
	defer s.mu.Unlock()

	email := strings.ToLower(cfg.SeedAdminEmail)
	if _, ok := s.usersByEmail[email]; ok {
		return nil
	}

	now := time.Now().UTC()
	admin := auth.User{
		ID:           shared.NewID("user"),
		Email:        cfg.SeedAdminEmail,
		Name:         "Platform Admin",
		PasswordHash: shared.HashPassword(cfg.SeedAdminPassword),
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

	s.users[admin.ID] = admin
	s.usersByEmail[email] = admin.ID
	s.workspaces[ws.ID] = ws
	s.memberships[member.ID] = member
	return nil
}

func (s *Store) FindUserByEmail(email string) (auth.User, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	if id, ok := s.usersByEmail[strings.ToLower(email)]; ok {
		return s.users[id], nil
	}
	return auth.User{}, errors.New("user not found")
}

func (s *Store) FindUserByID(userID string) (auth.User, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	item, ok := s.users[userID]
	if !ok {
		return auth.User{}, errors.New("user not found")
	}
	return item, nil
}

func (s *Store) SaveSessionToken(token auth.SessionToken) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.tokens[token.Token] = token
	return nil
}

func (s *Store) FindSessionToken(token string) (auth.SessionToken, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	item, ok := s.tokens[token]
	if !ok {
		return auth.SessionToken{}, errors.New("token not found")
	}
	return item, nil
}

func (s *Store) ListWorkspacesByUser(userID string) ([]workspace.Workspace, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	items := make([]workspace.Workspace, 0)
	for _, membership := range s.memberships {
		if membership.UserID == userID {
			if ws, ok := s.workspaces[membership.WorkspaceID]; ok {
				items = append(items, ws)
			}
		}
	}
	return items, nil
}

func (s *Store) UserInWorkspace(userID, workspaceID string) (bool, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	for _, membership := range s.memberships {
		if membership.UserID == userID && membership.WorkspaceID == workspaceID {
			return true, nil
		}
	}
	return false, nil
}

func (s *Store) CreateWorkspace(item workspace.Workspace, member workspace.Membership) error {
	s.mu.Lock()
	defer s.mu.Unlock()

	s.workspaces[item.ID] = item
	s.memberships[member.ID] = member
	return nil
}

func (s *Store) SaveAgent(item agent.Agent) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.agents[item.ID] = item
	return nil
}

func (s *Store) UpdateAgent(item agent.Agent) error {
	return s.SaveAgent(item)
}

func (s *Store) FindAgentByID(agentID string) (agent.Agent, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	item, ok := s.agents[agentID]
	if !ok {
		return agent.Agent{}, errors.New("agent not found")
	}
	return item, nil
}

func (s *Store) ListAgents(workspaceID string) ([]agent.Agent, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	items := make([]agent.Agent, 0)
	for _, item := range s.agents {
		if item.WorkspaceID == workspaceID {
			items = append(items, item)
		}
	}
	return items, nil
}

func (s *Store) CountAgentVersions(agentID string) (int, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	count := 0
	for _, item := range s.agentVersions {
		if item.AgentID == agentID {
			count++
		}
	}
	return count, nil
}

func (s *Store) SaveAgentVersion(item agent.Version) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.agentVersions[item.ID] = item
	return nil
}

func (s *Store) FindAgentVersion(versionID string) (agent.Version, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	item, ok := s.agentVersions[versionID]
	if !ok {
		return agent.Version{}, errors.New("agent version not found")
	}
	return item, nil
}

func (s *Store) SaveTool(item tool.Tool) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.tools[item.ID] = item
	return nil
}

func (s *Store) ListTools(workspaceID string) ([]tool.Tool, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	items := make([]tool.Tool, 0)
	for _, item := range s.tools {
		if item.WorkspaceID == workspaceID {
			items = append(items, item)
		}
	}
	return items, nil
}

func (s *Store) FindToolByID(toolID string) (tool.Tool, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	item, ok := s.tools[toolID]
	if !ok {
		return tool.Tool{}, errors.New("tool not found")
	}
	return item, nil
}

func (s *Store) SaveSession(item session.Session) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.sessions[item.ID] = item
	return nil
}

func (s *Store) SaveMessage(item session.Message) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.messages[item.SessionID] = append(s.messages[item.SessionID], item)
	return nil
}

func (s *Store) ListMessages(sessionID string) ([]session.Message, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	items := append([]session.Message(nil), s.messages[sessionID]...)
	return items, nil
}

func (s *Store) SaveSchedule(item task.Schedule) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.schedules[item.ID] = item
	return nil
}

func (s *Store) ListDueSchedules(now time.Time) ([]task.Schedule, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	items := make([]task.Schedule, 0)
	for _, item := range s.schedules {
		if !item.NextRunAt.After(now) {
			items = append(items, item)
		}
	}
	return items, nil
}

func (s *Store) UpdateSchedule(item task.Schedule) error {
	return s.SaveSchedule(item)
}

func (s *Store) SaveTask(item task.Task) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.tasks[item.ID] = item
	return nil
}

func (s *Store) UpdateTask(item task.Task) error {
	return s.SaveTask(item)
}

func (s *Store) FindTaskByID(taskID string) (task.Task, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	item, ok := s.tasks[taskID]
	if !ok {
		return task.Task{}, errors.New("task not found")
	}
	return item, nil
}

func (s *Store) SaveTaskStep(item task.Step) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.taskSteps[item.TaskID] = append(s.taskSteps[item.TaskID], item)
	return nil
}

func (s *Store) SaveTaskSteps(items []task.Step) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	for _, item := range items {
		s.taskSteps[item.TaskID] = append(s.taskSteps[item.TaskID], item)
	}
	return nil
}

func (s *Store) ReplaceTaskSteps(taskID string, items []task.Step) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.taskSteps[taskID] = append([]task.Step(nil), items...)
	return nil
}

func (s *Store) ListTaskSteps(taskID string) ([]task.Step, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	items := append([]task.Step(nil), s.taskSteps[taskID]...)
	return items, nil
}

func (s *Store) SaveApproval(item approval.Approval) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.approvals[item.ID] = item
	return nil
}

func (s *Store) UpdateApproval(item approval.Approval) error {
	return s.SaveApproval(item)
}

func (s *Store) FindApprovalByID(approvalID string) (approval.Approval, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	item, ok := s.approvals[approvalID]
	if !ok {
		return approval.Approval{}, errors.New("approval not found")
	}
	return item, nil
}

func (s *Store) ListApprovals(workspaceID string) ([]approval.Approval, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	items := make([]approval.Approval, 0)
	for _, item := range s.approvals {
		if workspaceID == "" || item.WorkspaceID == workspaceID {
			items = append(items, item)
		}
	}
	return items, nil
}

func (s *Store) SaveAuditEvent(item audit.Event) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.auditEvents = append(s.auditEvents, item)
	return nil
}

func (s *Store) ListAuditEvents(workspaceID string) ([]audit.Event, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()

	items := make([]audit.Event, 0)
	for _, item := range s.auditEvents {
		if workspaceID == "" || item.WorkspaceID == workspaceID {
			items = append(items, item)
		}
	}
	return items, nil
}
