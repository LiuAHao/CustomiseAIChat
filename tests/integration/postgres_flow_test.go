package integration

import (
	"context"
	"fmt"
	"net/http"
	"net/http/httptest"
	"net/url"
	"os"
	"strings"
	"testing"
	"time"

	"go-agent-platform/internal/app"
	"go-agent-platform/internal/config"
	httptransport "go-agent-platform/internal/transport/http"

	"github.com/jackc/pgx/v5"
	"github.com/jackc/pgx/v5/pgxpool"
)

func TestPostgresAuthAndTaskFlow(t *testing.T) {
	if os.Getenv("POSTGRES_INTEGRATION") != "1" {
		t.Skip("skip postgres integration; set POSTGRES_INTEGRATION=1")
	}

	dsn := os.Getenv("POSTGRES_TEST_DSN")
	if dsn == "" {
		dsn = os.Getenv("POSTGRES_DSN")
	}
	if dsn == "" {
		t.Skip("skip postgres integration; POSTGRES_TEST_DSN is empty")
	}

	ensurePostgresDatabase(t, dsn)
	resetPostgres(t, dsn)

	cfg := config.Load()
	cfg.StorageDriver = "postgres"
	cfg.PostgresDSN = dsn
	cfg.PostgresAutoMigrate = true

	application, err := app.New(cfg)
	if err != nil {
		t.Fatalf("bootstrap postgres app failed: %v", err)
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
}

func ensurePostgresDatabase(t *testing.T, dsn string) {
	t.Helper()

	targetURL, err := url.Parse(dsn)
	if err != nil {
		t.Fatalf("parse postgres dsn failed: %v", err)
	}
	dbName := strings.TrimPrefix(targetURL.Path, "/")
	if dbName == "" {
		t.Fatal("postgres dsn missing database name")
	}

	adminURL := *targetURL
	adminURL.Path = "/postgres"

	ctx, cancel := context.WithTimeout(context.Background(), 20*time.Second)
	defer cancel()

	pool, err := pgxpool.New(ctx, adminURL.String())
	if err != nil {
		t.Fatalf("connect admin postgres failed: %v", err)
	}
	defer pool.Close()

	var exists bool
	if err := pool.QueryRow(ctx, `select exists(select 1 from pg_database where datname = $1)`, dbName).Scan(&exists); err != nil {
		t.Fatalf("check postgres database failed: %v", err)
	}
	if exists {
		return
	}

	quotedName := pgx.Identifier{dbName}.Sanitize()
	if _, err := pool.Exec(ctx, fmt.Sprintf("create database %s", quotedName)); err != nil {
		t.Fatalf("create postgres database failed: %v", err)
	}
}

func resetPostgres(t *testing.T, dsn string) {
	t.Helper()

	ctx, cancel := context.WithTimeout(context.Background(), 20*time.Second)
	defer cancel()

	pool, err := pgxpool.New(ctx, dsn)
	if err != nil {
		t.Fatalf("connect postgres failed: %v", err)
	}
	defer pool.Close()

	statements := []string{
		`create table if not exists users (id text primary key, email text not null unique, name text not null, password_hash text not null, created_at timestamptz not null default now())`,
		`create table if not exists auth_sessions (token text primary key, user_id text not null references users(id) on delete cascade, expires_at timestamptz not null)`,
		`create table if not exists workspaces (id text primary key, name text not null, created_by text not null references users(id), created_at timestamptz not null default now())`,
		`create table if not exists memberships (id text primary key, workspace_id text not null references workspaces(id) on delete cascade, user_id text not null references users(id) on delete cascade, role text not null, created_at timestamptz not null default now())`,
		`create table if not exists agents (id text primary key, workspace_id text not null references workspaces(id) on delete cascade, name text not null, description text not null default '', system_prompt text not null default '', model text not null, tool_policy jsonb not null default '[]'::jsonb, runtime_policy text not null default '', published_version_id text, created_by text not null references users(id), created_at timestamptz not null default now(), updated_at timestamptz not null default now())`,
		`create table if not exists agent_versions (id text primary key, agent_id text not null references agents(id) on delete cascade, version_number integer not null, snapshot jsonb not null, created_by text not null references users(id), created_at timestamptz not null default now())`,
		`create table if not exists tools (id text primary key, workspace_id text not null references workspaces(id) on delete cascade, name text not null, description text not null default '', schema jsonb not null default '{}'::jsonb, config jsonb not null default '{}'::jsonb, requires_approval boolean not null default false, enabled boolean not null default true, created_by text not null references users(id), created_at timestamptz not null default now())`,
		`create table if not exists sessions (id text primary key, workspace_id text not null references workspaces(id) on delete cascade, agent_id text not null references agents(id) on delete cascade, created_by text not null references users(id), title text not null, created_at timestamptz not null default now(), updated_at timestamptz not null default now())`,
		`create table if not exists messages (id text primary key, session_id text not null references sessions(id) on delete cascade, role text not null, content text not null, trace_id text not null, created_at timestamptz not null default now())`,
		`create table if not exists tasks (id text primary key, trace_id text not null, workspace_id text not null references workspaces(id) on delete cascade, agent_id text not null references agents(id) on delete cascade, session_id text not null references sessions(id) on delete cascade, prompt text not null, status text not null, result text not null default '', error text not null default '', created_by text not null references users(id), approval_id text, tool_calls jsonb not null default '[]'::jsonb, created_at timestamptz not null default now(), updated_at timestamptz not null default now())`,
		`create table if not exists task_steps (id text primary key, task_id text not null references tasks(id) on delete cascade, name text not null, status text not null, output text not null default '', error text not null default '', sequence integer not null, approval_ref text not null default '', created_at timestamptz not null default now(), updated_at timestamptz not null default now())`,
		`create table if not exists approvals (id text primary key, workspace_id text not null references workspaces(id) on delete cascade, task_id text not null references tasks(id) on delete cascade, step_id text not null, reason text not null, status text not null, decision_by text not null default '', decision_at timestamptz, created_at timestamptz not null default now())`,
		`create table if not exists schedules (id text primary key, workspace_id text not null references workspaces(id) on delete cascade, agent_id text not null references agents(id) on delete cascade, name text not null, prompt text not null, cron text not null, interval text not null, next_run_at timestamptz not null, created_by text not null references users(id), created_at timestamptz not null default now())`,
		`create table if not exists audit_events (id text primary key, workspace_id text not null default '', actor_id text not null default '', action text not null, resource text not null, resource_id text not null, metadata jsonb not null default '{}'::jsonb, created_at timestamptz not null default now())`,
		`truncate table audit_events, approvals, task_steps, tasks, messages, sessions, schedules, tools, agent_versions, agents, memberships, workspaces, auth_sessions, users restart identity cascade`,
	}

	for _, stmt := range statements {
		if _, err := pool.Exec(ctx, stmt); err != nil {
			t.Fatalf("reset postgres failed: %v", err)
		}
	}
}
