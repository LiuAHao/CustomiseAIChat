# Go 多 Agent 管理平台实施说明

当前仓库已新增 Go 平台骨架，采用模块化单体结构，围绕认证、工作空间、Agent 管理、工具管理、任务编排、审批、调度、审计和 WebSocket 事件流实现了首版主链路。

当前版本重点是把新平台从 0 搭起来，并与旧 C++/Python 运行时解耦：

- `cmd/api`：HTTP API 与 `/ws` 事件推送入口
- `cmd/worker`：调度消费骨架
- `internal/app`：应用装配与核心业务编排
- `internal/domain/*`：领域对象定义
- `internal/platform/*`：内存存储、事件总线、LLM mock
- `internal/transport/*`：REST 与 WebSocket 传输层
- `web/console`：React 管理台骨架
- `migrations`：PostgreSQL schema 草案
- `deployments/docker-compose`：本地部署模板

首版实现说明：

- 存储层当前使用内存实现，便于在没有数据库驱动依赖的前提下快速跑通平台主流程
- 调度表达式当前支持 `@every <duration>` 形式，例如 `@every 30s`
- 工具执行与 LLM 调用当前使用 mock provider，已经保留正式 provider 接口
- 审批流、任务状态机、会话消息沉淀、事件推送已经串联

下一步建议：

1. 将 `internal/platform/memory` 替换为 PostgreSQL/Redis 仓储实现
2. 补充正式 JWT、RBAC、输入校验与 OpenAPI 生成
3. 接入真实 LLM provider 与真实 tool runner
4. 完善 React 管理台页面与交互
