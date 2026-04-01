package integration

import (
	"bytes"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"testing"

	"go-agent-platform/internal/app"
	"go-agent-platform/internal/config"
	httptransport "go-agent-platform/internal/transport/http"
)

func TestTaskApprovalHTTPFlow(t *testing.T) {
	application, err := app.New(config.Load())
	if err != nil {
		t.Fatalf("bootstrap app failed: %v", err)
	}
	defer application.Close()
	server := httptest.NewServer(httptransport.NewHandler(application))
	defer server.Close()

	token, workspaceID := loginAndGetWorkspace(t, server.URL)

	agentID := createAgentAndPublish(t, server.URL, token, workspaceID)
	toolID := createApprovalTool(t, server.URL, token, workspaceID)
	taskID := createApprovalTask(t, server.URL, token, workspaceID, agentID, toolID)

	taskDetail := getTask(t, server.URL, token, taskID)
	if taskDetail.Task.Status != "waiting_approval" {
		t.Fatalf("expected waiting_approval, got %s", taskDetail.Task.Status)
	}

	approvalsResp := authorizedRequest(t, http.MethodGet, server.URL+"/api/v1/approvals?workspace_id="+workspaceID, token, nil)
	defer approvalsResp.Body.Close()
	if approvalsResp.StatusCode != http.StatusOK {
		t.Fatalf("expected 200 from approvals, got %d", approvalsResp.StatusCode)
	}

	var approvalsPayload struct {
		Items []struct {
			ID string `json:"id"`
		} `json:"items"`
	}
	if err := json.NewDecoder(approvalsResp.Body).Decode(&approvalsPayload); err != nil {
		t.Fatalf("decode approvals failed: %v", err)
	}
	if len(approvalsPayload.Items) != 1 {
		t.Fatalf("expected 1 approval, got %d", len(approvalsPayload.Items))
	}

	approveResp := authorizedRequest(t, http.MethodPost, server.URL+"/api/v1/approvals/"+approvalsPayload.Items[0].ID+"/approve", token, nil)
	defer approveResp.Body.Close()
	if approveResp.StatusCode != http.StatusOK {
		t.Fatalf("expected 200 from approve, got %d", approveResp.StatusCode)
	}

	finalTask := getTask(t, server.URL, token, taskID)
	if finalTask.Task.Status != "completed" {
		t.Fatalf("expected completed, got %s", finalTask.Task.Status)
	}
}

func loginAndGetWorkspace(t *testing.T, baseURL string) (string, string) {
	t.Helper()

	loginBody := map[string]string{
		"email":    "admin@example.com",
		"password": "ChangeMe123!",
	}
	rawBody, err := json.Marshal(loginBody)
	if err != nil {
		t.Fatalf("marshal login failed: %v", err)
	}

	loginResp, err := http.Post(baseURL+"/api/v1/auth/login", "application/json", bytes.NewReader(rawBody))
	if err != nil {
		t.Fatalf("login request failed: %v", err)
	}
	defer loginResp.Body.Close()

	var payload struct {
		Token string `json:"token"`
	}
	if err := json.NewDecoder(loginResp.Body).Decode(&payload); err != nil {
		t.Fatalf("decode login response failed: %v", err)
	}

	meResp := authorizedRequest(t, http.MethodGet, baseURL+"/api/v1/me", payload.Token, nil)
	defer meResp.Body.Close()

	var mePayload struct {
		Workspaces []struct {
			ID string `json:"id"`
		} `json:"workspaces"`
	}
	if err := json.NewDecoder(meResp.Body).Decode(&mePayload); err != nil {
		t.Fatalf("decode /me failed: %v", err)
	}
	if len(mePayload.Workspaces) == 0 {
		t.Fatal("expected at least one workspace")
	}
	return payload.Token, mePayload.Workspaces[0].ID
}

func createAgentAndPublish(t *testing.T, baseURL, token, workspaceID string) string {
	t.Helper()

	agentBody := map[string]any{
		"workspace_id":   workspaceID,
		"name":           "Planner",
		"description":    "test planner",
		"system_prompt":  "Plan and execute",
		"model":          "mock-1",
		"tool_policy":    []string{},
		"runtime_policy": "default",
	}
	resp := authorizedRequest(t, http.MethodPost, baseURL+"/api/v1/agents", token, agentBody)
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusCreated {
		t.Fatalf("expected 201 from create agent, got %d", resp.StatusCode)
	}

	var agentPayload struct {
		ID string `json:"id"`
	}
	if err := json.NewDecoder(resp.Body).Decode(&agentPayload); err != nil {
		t.Fatalf("decode create agent failed: %v", err)
	}

	versionResp := authorizedRequest(t, http.MethodPost, baseURL+"/api/v1/agents/"+agentPayload.ID+"/versions", token, nil)
	defer versionResp.Body.Close()
	if versionResp.StatusCode != http.StatusCreated {
		t.Fatalf("expected 201 from create version, got %d", versionResp.StatusCode)
	}

	var versionPayload struct {
		ID string `json:"id"`
	}
	if err := json.NewDecoder(versionResp.Body).Decode(&versionPayload); err != nil {
		t.Fatalf("decode version failed: %v", err)
	}

	publishBody := map[string]string{"version_id": versionPayload.ID}
	publishResp := authorizedRequest(t, http.MethodPost, baseURL+"/api/v1/agents/"+agentPayload.ID+"/publish", token, publishBody)
	defer publishResp.Body.Close()
	if publishResp.StatusCode != http.StatusOK {
		t.Fatalf("expected 200 from publish, got %d", publishResp.StatusCode)
	}

	return agentPayload.ID
}

func createApprovalTool(t *testing.T, baseURL, token, workspaceID string) string {
	t.Helper()

	body := map[string]any{
		"workspace_id":      workspaceID,
		"name":              "write-email",
		"description":       "send email",
		"schema":            map[string]any{},
		"config":            map[string]any{},
		"requires_approval": true,
	}
	resp := authorizedRequest(t, http.MethodPost, baseURL+"/api/v1/tools", token, body)
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusCreated {
		t.Fatalf("expected 201 from create tool, got %d", resp.StatusCode)
	}

	var payload struct {
		ID string `json:"id"`
	}
	if err := json.NewDecoder(resp.Body).Decode(&payload); err != nil {
		t.Fatalf("decode tool failed: %v", err)
	}
	return payload.ID
}

func createApprovalTask(t *testing.T, baseURL, token, workspaceID, agentID, toolID string) string {
	t.Helper()

	body := map[string]any{
		"workspace_id": workspaceID,
		"agent_id":     agentID,
		"prompt":       "send report",
		"async":        false,
		"tool_calls": []map[string]any{
			{
				"tool_id": toolID,
				"input":   map[string]any{"to": "ops@example.com"},
			},
		},
	}
	resp := authorizedRequest(t, http.MethodPost, baseURL+"/api/v1/tasks", token, body)
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusCreated {
		t.Fatalf("expected 201 from create task, got %d", resp.StatusCode)
	}

	var payload struct {
		ID string `json:"id"`
	}
	if err := json.NewDecoder(resp.Body).Decode(&payload); err != nil {
		t.Fatalf("decode task failed: %v", err)
	}
	return payload.ID
}

func getTask(t *testing.T, baseURL, token, taskID string) struct {
	Task struct {
		Status string `json:"status"`
	} `json:"task"`
} {
	t.Helper()

	resp := authorizedRequest(t, http.MethodGet, baseURL+"/api/v1/tasks/"+taskID, token, nil)
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		t.Fatalf("expected 200 from get task, got %d", resp.StatusCode)
	}

	var payload struct {
		Task struct {
			Status string `json:"status"`
		} `json:"task"`
	}
	if err := json.NewDecoder(resp.Body).Decode(&payload); err != nil {
		t.Fatalf("decode task detail failed: %v", err)
	}
	return payload
}

func authorizedRequest(t *testing.T, method, url, token string, body any) *http.Response {
	t.Helper()

	var reader *bytes.Reader
	if body == nil {
		reader = bytes.NewReader(nil)
	} else {
		rawBody, err := json.Marshal(body)
		if err != nil {
			t.Fatalf("marshal request body failed: %v", err)
		}
		reader = bytes.NewReader(rawBody)
	}

	req, err := http.NewRequest(method, url, reader)
	if err != nil {
		t.Fatalf("build request failed: %v", err)
	}
	req.Header.Set("Authorization", "Bearer "+token)
	if body != nil {
		req.Header.Set("Content-Type", "application/json")
	}

	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		t.Fatalf("perform request failed: %v", err)
	}
	return resp
}
