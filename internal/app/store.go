package app

import (
	"time"

	"go-agent-platform/internal/config"
	"go-agent-platform/internal/domain/agent"
	"go-agent-platform/internal/domain/approval"
	"go-agent-platform/internal/domain/audit"
	"go-agent-platform/internal/domain/auth"
	"go-agent-platform/internal/domain/session"
	"go-agent-platform/internal/domain/task"
	"go-agent-platform/internal/domain/tool"
	"go-agent-platform/internal/domain/workspace"
)

// Store 定义应用层依赖的持久化能力，屏蔽 memory/postgres 差异。
type Store interface {
	Close() error
	EnsureSeedData(cfg config.Config) error
	FindUserByEmail(email string) (auth.User, error)
	FindUserByID(userID string) (auth.User, error)
	SaveSessionToken(token auth.SessionToken) error
	FindSessionToken(token string) (auth.SessionToken, error)
	ListWorkspacesByUser(userID string) ([]workspace.Workspace, error)
	UserInWorkspace(userID, workspaceID string) (bool, error)
	CreateWorkspace(item workspace.Workspace, member workspace.Membership) error
	SaveAgent(item agent.Agent) error
	UpdateAgent(item agent.Agent) error
	FindAgentByID(agentID string) (agent.Agent, error)
	ListAgents(workspaceID string) ([]agent.Agent, error)
	CountAgentVersions(agentID string) (int, error)
	SaveAgentVersion(item agent.Version) error
	FindAgentVersion(versionID string) (agent.Version, error)
	SaveTool(item tool.Tool) error
	ListTools(workspaceID string) ([]tool.Tool, error)
	FindToolByID(toolID string) (tool.Tool, error)
	SaveSession(item session.Session) error
	SaveMessage(item session.Message) error
	ListMessages(sessionID string) ([]session.Message, error)
	SaveSchedule(item task.Schedule) error
	ListDueSchedules(now time.Time) ([]task.Schedule, error)
	UpdateSchedule(item task.Schedule) error
	SaveTask(item task.Task) error
	UpdateTask(item task.Task) error
	FindTaskByID(taskID string) (task.Task, error)
	SaveTaskStep(item task.Step) error
	SaveTaskSteps(items []task.Step) error
	ReplaceTaskSteps(taskID string, items []task.Step) error
	ListTaskSteps(taskID string) ([]task.Step, error)
	SaveApproval(item approval.Approval) error
	UpdateApproval(item approval.Approval) error
	FindApprovalByID(approvalID string) (approval.Approval, error)
	ListApprovals(workspaceID string) ([]approval.Approval, error)
	SaveAuditEvent(item audit.Event) error
	ListAuditEvents(workspaceID string) ([]audit.Event, error)
}
