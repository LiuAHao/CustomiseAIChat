package app

import (
	"testing"

	"go-agent-platform/internal/config"
	"go-agent-platform/internal/domain/auth"
	"go-agent-platform/internal/domain/tool"
)

func TestLoginAndTaskApprovalFlow(t *testing.T) {
	application := New(config.Load())
	loginResp, err := application.Login("admin@example.com", "ChangeMe123!")
	if err != nil {
		t.Fatalf("login failed: %v", err)
	}
	user := loginResp["user"].(auth.User)
	userID := user.ID
	workspaces := application.ListWorkspaces(userID)
	if len(workspaces) == 0 {
		t.Fatalf("expected seeded workspace")
	}

	agentItem, err := application.CreateAgent(userID, CreateAgentRequest{
		WorkspaceID:  workspaces[0].ID,
		Name:         "Planner",
		Description:  "Task planner",
		SystemPrompt: "Plan and execute",
		Model:        "mock-1",
	})
	if err != nil {
		t.Fatalf("create agent failed: %v", err)
	}
	version, err := application.CreateVersion(userID, agentItem.ID)
	if err != nil {
		t.Fatalf("create version failed: %v", err)
	}
	if err := application.PublishVersion(userID, agentItem.ID, version.ID); err != nil {
		t.Fatalf("publish failed: %v", err)
	}
	toolItem, err := application.CreateTool(userID, CreateToolRequest{
		WorkspaceID:      workspaces[0].ID,
		Name:             "write-email",
		Description:      "sends email",
		RequiresApproval: true,
	})
	if err != nil {
		t.Fatalf("create tool failed: %v", err)
	}
	taskItem, err := application.ExecuteTask(userID, ExecuteTaskRequest{
		WorkspaceID: workspaces[0].ID,
		AgentID:     agentItem.ID,
		Prompt:      "send report",
		ToolCalls:   []tool.CallSpec{{ToolID: toolItem.ID, Input: map[string]any{"to": "ops@example.com"}}},
	})
	if err != nil {
		t.Fatalf("execute task failed: %v", err)
	}
	if taskItem.Status != "waiting_approval" {
		t.Fatalf("expected waiting approval, got %s", taskItem.Status)
	}
	approvals := application.ListApprovals(workspaces[0].ID)
	if len(approvals) != 1 {
		t.Fatalf("expected one approval")
	}
	if _, err := application.Approve(userID, approvals[0].ID, true); err != nil {
		t.Fatalf("approve failed: %v", err)
	}
	finalTask, err := application.GetTask(taskItem.ID)
	if err != nil {
		t.Fatalf("get task failed: %v", err)
	}
	if finalTask.Status != "completed" {
		t.Fatalf("expected completed, got %s", finalTask.Status)
	}
}
