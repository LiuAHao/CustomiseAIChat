# Go Agent Platform

一个面向公共场景的多 Agent 平台。

当前版本的核心方向是：

- 平台统一提供 `Skill` 和 `MCP`
- 用户可以安装平台资源到“我的 Skill / 我的 MCP”
- 用户也可以创建自己的私有 `Skill / MCP`
- 每个 Agent 在创建或管理时，只绑定用户当前可用的资源
- Agent 进入独立聊天页，在对话时按绑定配置调用模型、Skill 和 MCP

后端目前提供 `memory` 与 `postgres` 两种存储实现，前端控制台使用 React + Vite。

## 当前能力

已完成：

- 用户登录与鉴权
- Agent 创建、版本发布、任务执行、审批流
- 平台 Skill 目录 / 用户私有 Skill / 安装关系
- 平台 MCP 目录 / 用户私有 MCP / 安装关系
- 模型注册表
- Agent 会话与消息读取
- React 控制台：新建 Agent、Skill/MCP 管理、模型管理、Agent 聊天页

当前仍在演进：

- 更细粒度的权限与多租户能力
- 更完整的模型管理策略
- 更丰富的对话态能力，例如历史会话筛选、会话重命名
- 生产级缓存、限流、任务队列与观测能力

## 目录结构

```text
go-agent-platform/
├─ cmd/                      # API / Worker 入口
├─ internal/
│  ├─ app/                   # 应用层
│  ├─ config/                # 配置
│  ├─ domain/                # 领域模型
│  ├─ platform/
│  │  ├─ events/             # 事件总线
│  │  ├─ llm/                # LLM 抽象与 mock
│  │  ├─ memory/             # 内存存储
│  │  └─ postgres/           # PostgreSQL 存储
│  └─ transport/             # HTTP / WebSocket
├─ migrations/               # 数据库迁移
├─ scripts/                  # 启动、测试、构建脚本
├─ tests/                    # 集成测试
└─ web/console/              # 前端控制台
```

## Quick Start

### 1. 启动后端

默认使用内存存储：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\dev.ps1
```

后端默认地址：

- API: `http://localhost:8081`

默认管理员账号：

- 邮箱：`admin@example.com`
- 密码：`ChangeMe123!`

### 2. 启动前端

进入前端目录并启动开发服务器：

```powershell
cd .\web\console
npm install
npm run dev
```

前端默认地址：

- Console: `http://localhost:5173`

### 3. 使用 PostgreSQL 运行

如果你想使用 PostgreSQL：

```powershell
$env:STORAGE_DRIVER = "postgres"
$env:POSTGRES_DSN = "postgres://agent:agent@127.0.0.1:5432/agent_platform?sslmode=disable"
$env:POSTGRES_AUTO_MIGRATE = "true"
powershell -ExecutionPolicy Bypass -File .\scripts\dev.ps1
```

### 4. 运行测试

单元与集成测试：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\test.ps1
```

仅 PostgreSQL 集成测试：

```powershell
$env:POSTGRES_DSN = "postgres://agent:agent@127.0.0.1:5432/agent_platform?sslmode=disable"
powershell -ExecutionPolicy Bypass -File .\scripts\test-postgres.ps1
```

## 前后端运行关系

建议本地开发时同时启动两个进程：

1. 在项目根目录运行后端 `scripts/dev.ps1`
2. 在 `web/console` 目录运行 `npm run dev`

前端会调用：

- `http://localhost:8081/api/v1`

如果你调整了后端监听地址，需要同步修改前端 [api.ts](C:\Users\26594\Desktop\项目文件\go-agent-platform\web\console\src\api.ts) 中的 `API_BASE`。

## 配置项

常用环境变量：

- `HTTP_ADDR`
- `STORAGE_DRIVER`
- `POSTGRES_DSN`
- `POSTGRES_AUTO_MIGRATE`
- `JWT_SECRET`
- `SEED_ADMIN_EMAIL`
- `SEED_ADMIN_PASSWORD`
- `WORKER_POLL_INTERVAL`

参考文件：

- [.env.example](C:\Users\26594\Desktop\项目文件\go-agent-platform\.env.example)

## 关键模块说明

可先阅读这些 README：

- [web/console/src/components/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\web\console\src\components\README.md)
- [internal/domain/skill/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\domain\skill\README.md)
- [internal/domain/tool/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\domain\tool\README.md)
- [internal/domain/model/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\domain\model\README.md)
- [migrations/README.md](C:\Users\26594\Desktop\项目文件\go-agent-platform\migrations\README.md)

## 当前实现边界

为了兼容现有后端链路，`workspace` 仍然保留在内部存储与部分服务逻辑中，但已经不再作为前端产品概念暴露。

当前产品视角更接近：

- 平台资源目录
- 用户个人资源与安装列表
- Agent 资源绑定
- Agent 聊天与执行

后续如果要继续推进大规模公共平台能力，建议优先补：

1. 用户级权限与资源配额
2. 平台资源审核与发布流程
3. 模型注册表的多层级管理能力
4. 会话检索、归档与审计能力
