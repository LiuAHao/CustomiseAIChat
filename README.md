# Go 多 Agent 管理平台

这是一个基于 Go 的多 Agent 管理平台重构项目。当前仓库已经完成平台骨架，并接入了可切换的持久化层：

- `memory`：用于快速开发和默认测试
- `postgres`：用于真实持久化运行

首版目标是先跑通平台主链路，再逐步补齐 Redis、RAG、MCP、Skill 和完整控制台能力。

## 最近更新

最近一轮主要更新如下：

- 完成应用层存储抽象，[internal/app/store.go](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\app\store.go)
- 完成 PostgreSQL 持久化接入，[internal/platform/postgres/store.go](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\platform\postgres\store.go)
- 新增 `auth_sessions` 表并补齐首版核心表结构，[migrations/001_init.sql](C:\Users\26594\Desktop\项目文件\go-agent-platform\migrations\001_init.sql)
- `api` / `worker` 支持按 `STORAGE_DRIVER` 切换存储实现
- 补齐 PostgreSQL 集成测试与自动建库能力，[tests/integration/postgres_flow_test.go](C:\Users\26594\Desktop\项目文件\go-agent-platform\tests\integration\postgres_flow_test.go)
- 补齐 PostgreSQL 测试脚本，[scripts/test-postgres.ps1](C:\Users\26594\Desktop\项目文件\go-agent-platform\scripts\test-postgres.ps1)

## 当前能力

已完成：

- Go 模块化单体工程结构
- API / Worker 双入口
- 用户、工作区、Agent、Version、Tool、Session、Task、Approval、Audit 领域模型
- REST API 与 WebSocket 事件流
- `memory` / `postgres` 双存储驱动
- PostgreSQL 自动迁移与默认管理员种子初始化
- 单元测试、HTTP 集成测试、PostgreSQL 集成测试入口
- 前端控制台首页骨架

未完成：

- Redis 缓存与锁
- 真实 LLM provider
- RAG 流水线
- MCP 接入
- Skill Registry 后端模型
- 完整前端多页面控制台

## 目录结构

```text
go-agent-platform/
├─ cmd/                      # API / Worker 入口
├─ internal/
│  ├─ app/                   # 应用层主链路
│  ├─ config/                # 配置
│  ├─ domain/                # 领域模型
│  ├─ platform/
│  │  ├─ events/             # 进程内事件总线
│  │  ├─ llm/                # LLM 抽象与 mock
│  │  ├─ memory/             # 内存存储
│  │  └─ postgres/           # PostgreSQL 持久化
│  └─ transport/             # HTTP / WebSocket
├─ migrations/               # 数据库迁移
├─ deployments/              # 部署草案
├─ scripts/                  # 启动、构建、测试脚本
├─ tests/                    # 工程化测试目录
├─ web/console/              # 前端控制台
├─ TESTING.md                # 测试说明
└─ 重构方案.md                # 当前重构方案
```

## 快速开始

### 1. 运行自动化测试

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\test.ps1
```

### 2. 启动后端

默认使用内存存储：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\dev.ps1
```

使用 PostgreSQL：

```powershell
$env:STORAGE_DRIVER = "postgres"
$env:POSTGRES_DSN = "postgres://postgres:123456@127.0.0.1:5432/agent_platform?sslmode=disable"
$env:POSTGRES_AUTO_MIGRATE = "true"
powershell -ExecutionPolicy Bypass -File .\scripts\dev.ps1
```

说明：

- DSN 里不要保留 `< >` 占位符
- 如果密码包含 `@`、`:`、`/` 等字符，需要先做 URL 编码

### 3. 运行 PostgreSQL 集成测试

```powershell
$env:POSTGRES_DSN = "postgres://postgres:123456@127.0.0.1:5432/agent_platform?sslmode=disable"
powershell -ExecutionPolicy Bypass -File .\scripts\test-postgres.ps1
```

测试脚本会：

- 优先读取当前环境变量里的 `POSTGRES_DSN`
- 自动检查并创建 `agent_platform` 数据库
- 自动建表并清理测试数据

### 4. Docker Compose 启动

```powershell
cd .\deployments\docker-compose
docker compose up
```

Compose 默认会让 `api` 和 `worker` 使用 `postgres` 驱动。

## 默认账号

- 邮箱：`admin@example.com`
- 密码：`ChangeMe123!`

## 配置项

常用环境变量：

- `STORAGE_DRIVER`：`memory` 或 `postgres`
- `POSTGRES_DSN`：PostgreSQL 连接串
- `POSTGRES_AUTO_MIGRATE`：是否自动执行 [001_init.sql](C:\Users\26594\Desktop\项目文件\go-agent-platform\migrations\001_init.sql)
- `HTTP_ADDR`：API 监听地址
- `WORKER_POLL_INTERVAL`：worker 调度轮询间隔

参考文件：[.env.example](C:\Users\26594\Desktop\项目文件\go-agent-platform\.env.example)

## 数据库设计

首版数据库以“工作区隔离 + 平台主链路可追踪”为目标，核心表如下：

### 身份与租户

- `users`
  平台用户
- `auth_sessions`
  登录态和 token
- `workspaces`
  工作区
- `memberships`
  用户与工作区关系

### Agent 与工具配置

- `agents`
  Agent 当前配置与发布指针
- `agent_versions`
  Agent 配置快照
- `tools`
  工具定义、schema、配置与审批要求

### 会话与执行

- `sessions`
  会话容器
- `messages`
  会话消息
- `tasks`
  一次任务执行请求
- `task_steps`
  任务步骤轨迹
- `approvals`
  审批节点
- `schedules`
  调度任务

### 审计

- `audit_events`
  配置变更、任务执行、审批决策等审计日志

### 关系概览

```text
users
├─ auth_sessions
├─ workspaces.created_by
└─ memberships.user_id

workspaces
├─ memberships.workspace_id
├─ agents.workspace_id
├─ tools.workspace_id
├─ sessions.workspace_id
├─ tasks.workspace_id
├─ approvals.workspace_id
└─ schedules.workspace_id

agents
├─ agent_versions.agent_id
├─ sessions.agent_id
├─ tasks.agent_id
└─ schedules.agent_id

sessions
├─ messages.session_id
└─ tasks.session_id

tasks
├─ task_steps.task_id
└─ approvals.task_id
```

完整 SQL 见：[migrations/001_init.sql](C:\Users\26594\Desktop\项目文件\go-agent-platform\migrations\001_init.sql)

## 模块说明索引

后端关键目录已补充模块说明：

- [cmd/api/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\cmd\api\README.md)
- [cmd/worker/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\cmd\worker\README.md)
- [internal/app/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\app\README.md)
- [internal/config/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\config\README.md)
- [internal/domain/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\domain\README.md)
- [internal/platform/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\platform\README.md)
- [internal/platform/memory/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\platform\memory\README.md)
- [internal/platform/postgres/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\platform\postgres\README.md)
- [internal/transport/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\transport\README.md)
- [migrations/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\migrations\README.md)
- [tests/integration/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\tests\integration\README.md)

## 测试说明

测试入口：

- [scripts/test-unit.ps1](C:\Users\26594\Desktop\项目文件\go-agent-platform\scripts\test-unit.ps1)
- [scripts/test-integration.ps1](C:\Users\26594\Desktop\项目文件\go-agent-platform\scripts\test-integration.ps1)
- [scripts/test-postgres.ps1](C:\Users\26594\Desktop\项目文件\go-agent-platform\scripts\test-postgres.ps1)
- [scripts/smoke.ps1](C:\Users\26594\Desktop\项目文件\go-agent-platform\scripts\smoke.ps1)

详细说明见 [TESTING.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\TESTING.md)。

## 当前建议的下一步

1. 给 PostgreSQL 任务、审批、消息链路补事务，降低中间态风险。
2. 把 [internal/platform/postgres/store.go](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\platform\postgres\store.go) 继续拆分成更细的仓储文件。
3. 接入 Redis，用于 token、热点状态和分布式锁。
4. 继续推进真实 provider、RAG、MCP 和 Skill 能力。
