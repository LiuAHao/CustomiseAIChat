package approval

import "time"

type Status string

const (
	StatusPending  Status = "pending"
	StatusApproved Status = "approved"
	StatusRejected Status = "rejected"
	StatusExpired  Status = "expired"
)

type Approval struct {
	ID          string    `json:"id"`
	WorkspaceID string    `json:"workspace_id"`
	TaskID      string    `json:"task_id"`
	StepID      string    `json:"step_id"`
	Reason      string    `json:"reason"`
	Status      Status    `json:"status"`
	DecisionBy  string    `json:"decision_by,omitempty"`
	DecisionAt  time.Time `json:"decision_at,omitempty"`
	CreatedAt   time.Time `json:"created_at"`
}
