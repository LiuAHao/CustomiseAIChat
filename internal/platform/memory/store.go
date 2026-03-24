package memory

import (
	"sync"

	"go-agent-platform/internal/domain/agent"
	"go-agent-platform/internal/domain/approval"
	"go-agent-platform/internal/domain/audit"
	"go-agent-platform/internal/domain/auth"
	"go-agent-platform/internal/domain/session"
	"go-agent-platform/internal/domain/task"
	"go-agent-platform/internal/domain/tool"
	"go-agent-platform/internal/domain/workspace"
)

type Store struct {
	mu sync.RWMutex

	Users          map[string]auth.User
	UsersByEmail   map[string]string
	Tokens         map[string]auth.SessionToken
	Workspaces     map[string]workspace.Workspace
	Memberships    map[string]workspace.Membership
	Agents         map[string]agent.Agent
	AgentVersions  map[string]agent.Version
	Tools          map[string]tool.Tool
	Sessions       map[string]session.Session
	Messages       map[string][]session.Message
	Tasks          map[string]task.Task
	TaskSteps      map[string][]task.Step
	Approvals      map[string]approval.Approval
	AuditEvents    []audit.Event
	Schedules      map[string]task.Schedule
}

func NewStore() *Store {
	return &Store{
		Users:         make(map[string]auth.User),
		UsersByEmail:  make(map[string]string),
		Tokens:        make(map[string]auth.SessionToken),
		Workspaces:    make(map[string]workspace.Workspace),
		Memberships:   make(map[string]workspace.Membership),
		Agents:        make(map[string]agent.Agent),
		AgentVersions: make(map[string]agent.Version),
		Tools:         make(map[string]tool.Tool),
		Sessions:      make(map[string]session.Session),
		Messages:      make(map[string][]session.Message),
		Tasks:         make(map[string]task.Task),
		TaskSteps:     make(map[string][]task.Step),
		Approvals:     make(map[string]approval.Approval),
		Schedules:     make(map[string]task.Schedule),
	}
}

func (s *Store) WithWrite(fn func()) {
	s.mu.Lock()
	defer s.mu.Unlock()
	fn()
}

func (s *Store) WithRead(fn func()) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	fn()
}
