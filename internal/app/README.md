# `internal/app`

这个目录是当前项目的“应用层核心”。

## 作用

`internal/app` 负责把多个领域对象和基础设施拼起来，形成真正可执行的业务用例。

可以把它理解为：

- 比 `domain` 更接近“系统动作”
- 比 `transport` 更独立于 HTTP / WebSocket
- 比 `platform` 更关注业务流程，而不是底层能力

## 当前文件

- `app.go`
- `app_test.go`

## 这里解决的问题

当前 `Application` 统一封装了这些能力：

- 登录与鉴权
- 工作区创建与查询
- Agent 创建、版本化、发布
- Tool 创建与查询
- Session 创建与消息沉淀
- Task 创建、执行、取消
- Approval 审批与恢复执行
- Schedule 创建与轮询触发
- Audit 事件记录
- Event Hub 广播

## 为什么当前很多逻辑都在这里

现在项目处于“平台骨架阶段”，优先目标是把主链路跑通。为了减少文件碎片和跨包跳转，很多流程被集中放在 `Application` 中。

这适合早期，但不适合长期扩张。

## 后续重构建议

后续可以把 `app.go` 逐步拆成更明确的应用服务，例如：

- `AuthService`
- `AgentService`
- `ToolService`
- `TaskService`
- `ApprovalService`
- `ScheduleService`

拆分原则是：当某个流程开始拥有稳定边界、独立测试需求、或跨文件依赖明显增加时，就该从 `Application` 中抽走。
