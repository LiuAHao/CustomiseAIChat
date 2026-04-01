# 测试说明

本文档说明当前仓库的测试分层、测试入口和推荐执行流程。

## 测试分层

### 1. 单元测试

目标：

- 验证 `internal/app` 主链路行为
- 不依赖真实 HTTP 服务
- 默认使用 `memory` 存储驱动

当前入口：

- [internal/app/app_test.go](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\app\app_test.go)
- [scripts/test-unit.ps1](C:\Users\26594\Desktop\项目文件\go-agent-platform\scripts\test-unit.ps1)

### 2. HTTP 集成测试

目标：

- 使用 `httptest` 启动真实 handler
- 验证登录、鉴权、Agent、Task、Approval 等接口链路
- 默认仍使用 `memory` 存储驱动

当前入口：

- [tests/integration/auth_flow_test.go](C:\Users\26594\Desktop\项目文件\go-agent-platform\tests\integration\auth_flow_test.go)
- [tests/integration/task_flow_test.go](C:\Users\26594\Desktop\项目文件\go-agent-platform\tests\integration\task_flow_test.go)
- [scripts/test-integration.ps1](C:\Users\26594\Desktop\项目文件\go-agent-platform\scripts\test-integration.ps1)

### 3. PostgreSQL 集成测试

目标：

- 验证 `postgres` 驱动可以完成种子初始化、HTTP 登录、任务与审批流程
- 使用真实 PostgreSQL 数据库

当前入口：

- [tests/integration/postgres_flow_test.go](C:\Users\26594\Desktop\项目文件\go-agent-platform\tests\integration\postgres_flow_test.go)
- [scripts/test-postgres.ps1](C:\Users\26594\Desktop\项目文件\go-agent-platform\scripts\test-postgres.ps1)

默认情况下，这类测试不会自动运行，需要显式设置：

- `POSTGRES_INTEGRATION=1`
- `POSTGRES_TEST_DSN=<你的测试库连接串>`

### 4. Smoke Test

目标：

- 验证系统能否启动
- 验证健康检查与登录入口

当前入口：

- [scripts/smoke.ps1](C:\Users\26594\Desktop\项目文件\go-agent-platform\scripts\smoke.ps1)

## 推荐测试流程

### 日常开发

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\test.ps1
```

### 修改 API 或主链路后

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\test-all.ps1
```

### 验证 PostgreSQL 路径

先启动 PostgreSQL，再执行：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\test-postgres.ps1
```

### 启动后做冒烟验证

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\dev.ps1 -NoWorker
powershell -ExecutionPolicy Bypass -File .\scripts\smoke.ps1
```

## 当前覆盖重点

当前测试重点放在平台阶段 0 和持久化切换后的基础稳定性：

- 登录与鉴权
- 默认工作区种子初始化
- Agent 创建、版本发布
- Tool 注册
- Task 执行
- Approval 审批恢复
- PostgreSQL 接入主链路

## 后续建议补充

1. WebSocket 事件流集成测试
2. 调度触发测试
3. PostgreSQL 事务级一致性测试
4. Redis 接入后的缓存与锁测试
5. 前后端契约测试
