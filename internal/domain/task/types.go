package task

import (
	"time"

	"go-agent-platform/internal/domain/tool"
)

type Status string

const (
	StatusPending         Status = "pending"
	StatusRunning         Status = "running"
	StatusWaitingApproval Status = "waiting_approval"
	StatusCompleted       Status = "completed"
	StatusFailed          Status = "failed"
	StatusCanceled        Status = "canceled"
)

type StepStatus string

const (
	StepPending   StepStatus = "pending"
	StepRunning   StepStatus = "running"
	StepCompleted StepStatus = "completed"
	StepFailed    StepStatus = "failed"
	StepWaiting   StepStatus = "waiting_approval"
)

type Task struct {
	ID          string          `json:"id"`
	TraceID     string          `json:"trace_id"`
	WorkspaceID string          `json:"workspace_id"`
	AgentID     string          `json:"agent_id"`
	SessionID   string          `json:"session_id"`
	Prompt      string          `json:"prompt"`
	Status      Status          `json:"status"`
	Result      string          `json:"result,omitempty"`
	Error       string          `json:"error,omitempty"`
	CreatedBy   string          `json:"created_by"`
	ApprovalID  string          `json:"approval_id,omitempty"`
	ToolCalls   []tool.CallSpec `json:"tool_calls,omitempty"`
	CreatedAt   time.Time       `json:"created_at"`
	UpdatedAt   time.Time       `json:"updated_at"`
}

type Step struct {
	ID          string     `json:"id"`
	TaskID      string     `json:"task_id"`
	Name        string     `json:"name"`
	Status      StepStatus `json:"status"`
	Output      string     `json:"output,omitempty"`
	Error       string     `json:"error,omitempty"`
	Sequence    int        `json:"sequence"`
	ApprovalRef string     `json:"approval_ref,omitempty"`
	CreatedAt   time.Time  `json:"created_at"`
	UpdatedAt   time.Time  `json:"updated_at"`
}

type Schedule struct {
	ID          string    `json:"id"`
	WorkspaceID string    `json:"workspace_id"`
	AgentID     string    `json:"agent_id"`
	Name        string    `json:"name"`
	Prompt      string    `json:"prompt"`
	Cron        string    `json:"cron"`
	Interval    string    `json:"interval"`
	NextRunAt   time.Time `json:"next_run_at"`
	CreatedBy   string    `json:"created_by"`
	CreatedAt   time.Time `json:"created_at"`
}
