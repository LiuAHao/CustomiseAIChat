package app

import (
	"testing"

	"go-agent-platform/internal/config"
	"go-agent-platform/internal/domain/auth"
	"go-agent-platform/internal/domain/tool"
)

func TestLoginAndTaskApprovalFlow(t *testing.T) {
	application, err := New(config.Load())
	if err != nil {
		t.Fatalf("bootstrap app failed: %v", err)
	}
	defer application.Close()
	loginResp, err := application.Login("admin@example.com", "ChangeMe123!")
	if err != nil {
		t.Fatalf("login failed: %v", err)
	}
	user := loginResp["user"].(auth.User)
	userID := user.ID
	workspaceID, err := application.ResolveWorkspaceID(userID, "")
	if err != nil {
		t.Fatalf("resolve workspace failed: %v", err)
	}

	agentItem, err := application.CreateAgent(userID, CreateAgentRequest{
		WorkspaceID:  workspaceID,
		Name:         "Planner",
		Description:  "Task planner",
		SystemPrompt: "Plan and execute",
		Model:        "mock-1",
		SkillPolicy:  []string{},
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
		WorkspaceID:      workspaceID,
		Name:             "write-email",
		Description:      "sends email",
		RequiresApproval: true,
	})
	if err != nil {
		t.Fatalf("create tool failed: %v", err)
	}
	taskItem, err := application.ExecuteTask(userID, ExecuteTaskRequest{
		WorkspaceID: workspaceID,
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
	approvals := application.ListApprovals(workspaceID)
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

func TestSkillAndModelRegistryFlow(t *testing.T) {
	application, err := New(config.Load())
	if err != nil {
		t.Fatalf("bootstrap app failed: %v", err)
	}
	defer application.Close()

	loginResp, err := application.Login("admin@example.com", "ChangeMe123!")
	if err != nil {
		t.Fatalf("login failed: %v", err)
	}
	user := loginResp["user"].(auth.User)
	workspaceID, err := application.ResolveWorkspaceID(user.ID, "")
	if err != nil {
		t.Fatalf("resolve workspace failed: %v", err)
	}

	skillItem, err := application.CreateSkill(user.ID, CreateSkillRequest{
		WorkspaceID: workspaceID,
		Name:        "research-skill",
		Description: "collects research context",
		Entry:       "builtin/research",
	})
	if err != nil {
		t.Fatalf("create skill failed: %v", err)
	}

	modelItem, err := application.CreateModel(user.ID, CreateModelRequest{
		WorkspaceID: workspaceID,
		Name:        "GPT-4.1",
		Provider:    "openai",
		ModelKey:    "gpt-4.1",
		IsDefault:   true,
	})
	if err != nil {
		t.Fatalf("create model failed: %v", err)
	}

	agentItem, err := application.CreateAgent(user.ID, CreateAgentRequest{
		WorkspaceID:  workspaceID,
		Name:         "Industry Analyst",
		Description:  "tracks industry updates",
		SystemPrompt: "prioritize source quality",
		Model:        modelItem.ModelKey,
		SkillPolicy:  []string{skillItem.ID},
		ToolPolicy:   []string{},
	})
	if err != nil {
		t.Fatalf("create agent failed: %v", err)
	}

	if len(application.ListSkills(workspaceID)) != 1 {
		t.Fatalf("expected one skill")
	}
	models := application.ListModels(workspaceID)
	if len(models) < 2 {
		t.Fatalf("expected seeded default model plus created model")
	}
	found := false
	for _, item := range models {
		if item.ID == modelItem.ID {
			found = true
			break
		}
	}
	if !found {
		t.Fatalf("expected created model to be listed")
	}
	if len(agentItem.SkillPolicy) != 1 || agentItem.SkillPolicy[0] != skillItem.ID {
		t.Fatalf("expected agent skill policy to be persisted")
	}
}
