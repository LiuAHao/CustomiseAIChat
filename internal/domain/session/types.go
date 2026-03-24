package session

import "time"

type Session struct {
	ID          string    `json:"id"`
	WorkspaceID string    `json:"workspace_id"`
	AgentID     string    `json:"agent_id"`
	CreatedBy   string    `json:"created_by"`
	Title       string    `json:"title"`
	CreatedAt   time.Time `json:"created_at"`
	UpdatedAt   time.Time `json:"updated_at"`
}

type MessageRole string

const (
	RoleUser      MessageRole = "user"
	RoleAgent     MessageRole = "agent"
	RoleSystem    MessageRole = "system"
	RoleTool      MessageRole = "tool"
	RoleAssistant MessageRole = "assistant"
)

type Message struct {
	ID        string      `json:"id"`
	SessionID string      `json:"session_id"`
	Role      MessageRole `json:"role"`
	Content   string      `json:"content"`
	TraceID   string      `json:"trace_id"`
	CreatedAt time.Time   `json:"created_at"`
}
