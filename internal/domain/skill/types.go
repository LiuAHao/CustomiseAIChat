package skill

import "time"

const (
	ScopePlatform = "platform"
	ScopePersonal = "personal"
)

type Skill struct {
	ID          string         `json:"id"`
	WorkspaceID string         `json:"workspace_id"`
	Name        string         `json:"name"`
	Slug        string         `json:"slug"`
	Scope       string         `json:"scope"`
	Description string         `json:"description"`
	Version     string         `json:"version"`
	Entry       string         `json:"entry"`
	Schema      map[string]any `json:"schema"`
	Config      map[string]any `json:"config"`
	Enabled     bool           `json:"enabled"`
	CreatedBy   string         `json:"created_by"`
	CreatedAt   time.Time      `json:"created_at"`
	UpdatedAt   time.Time      `json:"updated_at"`
}
