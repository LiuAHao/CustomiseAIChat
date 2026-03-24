package tool

import "time"

type Tool struct {
	ID               string                 `json:"id"`
	WorkspaceID      string                 `json:"workspace_id"`
	Name             string                 `json:"name"`
	Description      string                 `json:"description"`
	Schema           map[string]any         `json:"schema"`
	Config           map[string]any         `json:"config"`
	RequiresApproval bool                   `json:"requires_approval"`
	Enabled          bool                   `json:"enabled"`
	CreatedBy        string                 `json:"created_by"`
	CreatedAt        time.Time              `json:"created_at"`
}

type CallSpec struct {
	ToolID string         `json:"tool_id"`
	Input  map[string]any `json:"input"`
}
