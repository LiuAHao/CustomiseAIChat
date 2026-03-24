package audit

import "time"

type Event struct {
	ID          string         `json:"id"`
	WorkspaceID string         `json:"workspace_id,omitempty"`
	ActorID     string         `json:"actor_id,omitempty"`
	Action      string         `json:"action"`
	Resource    string         `json:"resource"`
	ResourceID  string         `json:"resource_id"`
	Metadata    map[string]any `json:"metadata,omitempty"`
	CreatedAt   time.Time      `json:"created_at"`
}
