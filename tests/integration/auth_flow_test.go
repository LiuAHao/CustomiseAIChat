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

func TestAuthLoginAndMeFlow(t *testing.T) {
	application, err := app.New(config.Load())
	if err != nil {
		t.Fatalf("bootstrap app failed: %v", err)
	}
	defer application.Close()
	server := httptest.NewServer(httptransport.NewHandler(application))
	defer server.Close()

	loginBody := map[string]string{
		"email":    "admin@example.com",
		"password": "ChangeMe123!",
	}
	rawBody, err := json.Marshal(loginBody)
	if err != nil {
		t.Fatalf("marshal login body failed: %v", err)
	}

	loginResp, err := http.Post(server.URL+"/api/v1/auth/login", "application/json", bytes.NewReader(rawBody))
	if err != nil {
		t.Fatalf("login request failed: %v", err)
	}
	defer loginResp.Body.Close()

	if loginResp.StatusCode != http.StatusOK {
		t.Fatalf("expected 200, got %d", loginResp.StatusCode)
	}

	var payload struct {
		Token string `json:"token"`
		User  struct {
			Email string `json:"email"`
		} `json:"user"`
	}
	if err := json.NewDecoder(loginResp.Body).Decode(&payload); err != nil {
		t.Fatalf("decode login response failed: %v", err)
	}
	if payload.Token == "" {
		t.Fatal("expected token in login response")
	}
	if payload.User.Email != "admin@example.com" {
		t.Fatalf("expected admin user, got %s", payload.User.Email)
	}

	req, err := http.NewRequest(http.MethodGet, server.URL+"/api/v1/me", nil)
	if err != nil {
		t.Fatalf("build /me request failed: %v", err)
	}
	req.Header.Set("Authorization", "Bearer "+payload.Token)

	meResp, err := http.DefaultClient.Do(req)
	if err != nil {
		t.Fatalf("request /me failed: %v", err)
	}
	defer meResp.Body.Close()

	if meResp.StatusCode != http.StatusOK {
		t.Fatalf("expected 200 from /me, got %d", meResp.StatusCode)
	}
}
