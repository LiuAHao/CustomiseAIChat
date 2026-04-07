package agent

import "time"

type Agent struct {
	ID             string    `json:"id"`
	WorkspaceID    string    `json:"workspace_id"`
	Name           string    `json:"name"`
	Description    string    `json:"description"`
	SystemPrompt   string    `json:"system_prompt"`
	Model          string    `json:"model"`
	SkillPolicy    []string  `json:"skill_policy"`
	ToolPolicy     []string  `json:"tool_policy"`
	RuntimePolicy  string    `json:"runtime_policy"`
	PublishedVerID string    `json:"published_version_id,omitempty"`
	CreatedBy      string    `json:"created_by"`
	CreatedAt      time.Time `json:"created_at"`
	UpdatedAt      time.Time `json:"updated_at"`
}

type Version struct {
	ID            string                 `json:"id"`
	AgentID       string                 `json:"agent_id"`
	VersionNumber int                    `json:"version_number"`
	Snapshot      map[string]any         `json:"snapshot"`
	CreatedBy     string                 `json:"created_by"`
	CreatedAt     time.Time              `json:"created_at"`
}
