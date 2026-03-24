# 测试说明

本文档用于说明当前 Go 平台的测试目标、测试分层、测试文件放置规则，以及阶段 0 的标准验证流程。

---

## 1. 当前测试目标

阶段 0 的目标不是覆盖所有业务细节，而是确认平台骨架已经可运行、可登录、可执行任务主链路。

当前重点验证：

- Go 工程可以编译和测试
- API 可以启动
- 健康检查接口可用
- 默认管理员可以登录
- `Application` 主链路可运行
- Agent / Version / Tool / Task / Approval 的基本流程打通

---

## 2. 测试分层

当前项目建议按以下三层组织测试：

### 2.1 单元测试

目标：

- 直接测试 `internal/app` 或未来的应用服务
- 不依赖真实网络
- 不依赖真实数据库
- 使用内存存储快速验证主流程

当前示例：

- `internal/app/app_test.go`

适合覆盖：

- 登录
- 工作区创建
- Agent 创建 / 发布
- Tool 注册
- Task 执行
- Approval 流转

### 2.2 集成测试

目标：

- 启动真实 API
- 通过 HTTP 验证接口行为
- 验证 REST / 鉴权 / JSON 序列化链路

当前状态：

- 尚未形成独立测试文件
- 当前用 smoke 脚本完成手动或半自动验证

后续建议：

- 新建 `tests/integration/`
- 用标准 HTTP client 做接口级验证

### 2.3 Smoke Test

目标：

- 验证“系统能不能跑起来”
- 适合每次改完主链路后快速检查

当前建议检查项：

- 启动 API
- 调 `/healthz`
- 登录默认管理员

---

## 3. 测试文件规范

### 3.1 命名规范

- Go 单元测试文件：`*_test.go`
- PowerShell 验证脚本：放在 `scripts/`
- 测试说明文档：放在项目根目录或 `docs/`

### 3.2 放置规则

#### 应用层测试

放在被测包目录中，例如：

- `internal/app/app_test.go`

#### 未来集成测试

建议新增目录：

```text
tests/
└── integration/
```

例如：

- `tests/integration/auth_test.go`
- `tests/integration/task_flow_test.go`

#### 脚本型 smoke 测试

放在：

- `scripts/smoke.ps1`

### 3.3 编写原则

当前阶段建议遵循：

1. 优先验证主链路
2. 单个测试只覆盖一个清晰流程
3. 测试名直接表达业务含义
4. 先保证可读，再追求覆盖率

例如：

- `TestLoginAndTaskApprovalFlow`
- `TestCreateWorkspace`
- `TestPublishAgentVersion`

---

## 4. 当前可执行测试入口

### 4.1 运行全部 Go 测试

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\test.ps1
```

### 4.2 构建二进制

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1
```

### 4.3 启动 API

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\dev.ps1 -NoWorker
```

### 4.4 运行 smoke test

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\smoke.ps1
```

---

## 5. 阶段 0 标准验证流程

建议按以下顺序执行：

1. 运行单元测试

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\test.ps1
```

2. 构建二进制

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1
```

3. 启动 API

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\dev.ps1 -NoWorker
```

4. 新开终端执行 smoke test

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\smoke.ps1
```

5. 预期结果

- `/healthz` 返回 `{"status":"ok"}`
- 登录接口返回 token
- 控制台可使用默认账号登录

---

## 6. 默认测试账号

- 邮箱：`admin@example.com`
- 密码：`ChangeMe123!`

---

## 7. 后续测试增强建议

下一阶段建议逐步补齐以下测试：

- `internal/app` 的更多应用服务测试
- `internal/transport/http` 的接口级测试
- 真实 PostgreSQL 接入后的 repository 测试
- 任务状态机测试
- Approval 拒绝路径测试
- Schedule 触发测试
- WebSocket 事件推送测试

当前阶段不要求追求高覆盖率，优先保证：

- 主链路可运行
- 测试入口标准化
- 新人能按文档独立完成验证
