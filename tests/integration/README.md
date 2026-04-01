# Integration Tests

这个目录存放接口级和跨模块的集成测试，重点验证系统主链路在多个模块串联后是否仍然成立。

## 当前测试文件

- [auth_flow_test.go](C:\Users\26594\Desktop\项目文件\go-agent-platform\tests\integration\auth_flow_test.go)
  登录和 `/me` 接口验证
- [task_flow_test.go](C:\Users\26594\Desktop\项目文件\go-agent-platform\tests\integration\task_flow_test.go)
  Agent、Tool、Task、Approval HTTP 主链路验证
- [postgres_flow_test.go](C:\Users\26594\Desktop\项目文件\go-agent-platform\tests\integration\postgres_flow_test.go)
  真实 PostgreSQL 下的主链路验证

## 测试分组

### 默认集成测试

特点：

- 使用 `httptest` 启动真实 HTTP handler
- 默认走 `memory` 存储
- 不依赖外部数据库

适合日常开发快速回归。

### PostgreSQL 集成测试

特点：

- 使用真实 PostgreSQL
- 自动检查并创建测试库
- 自动建表并清理测试数据

适合验证持久化链路。

## 运行方式

默认集成测试：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\test-integration.ps1
```

PostgreSQL 集成测试：

```powershell
$env:POSTGRES_DSN = "postgres://postgres:123456@127.0.0.1:5432/agent_platform?sslmode=disable"
powershell -ExecutionPolicy Bypass -File .\scripts\test-postgres.ps1
```

## 后续待补

- WebSocket 事件流集成测试
- 调度触发测试
- 失败与重试路径测试
