package tool

import "time"

const (
	ScopePlatform = "platform"
	ScopePersonal = "personal"
)

type Tool struct {
	ID               string                 `json:"id"`
	WorkspaceID      string                 `json:"workspace_id"`
	Name             string                 `json:"name"`
	Scope            string                 `json:"scope"`
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
