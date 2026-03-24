# `internal/platform`

这个目录放底层平台能力实现。

## 作用

如果 `domain` 回答的是“系统讨论什么”，`platform` 回答的就是“系统依赖什么底层能力运行”。

当前子目录包括：

- `events`：进程内事件广播
- `llm`：LLM Provider 抽象与 mock 实现
- `memory`：当前内存存储实现
- `scheduler`：调度相关预留目录

## 当前设计思路

项目现在先用最小依赖跑通主流程，所以：

- 存储用内存实现
- LLM 用 mock provider
- 事件广播用进程内 hub

这样能在不接入外部基础设施的前提下，先验证平台主链路。

## 后续演进

后续这里会逐步接入真正的基础设施实现，例如：

- PostgreSQL 仓储
- Redis 缓存与锁
- 真正的 MQ / Job Queue
- 真实 LLM Provider
- 对象存储
- 指标、日志、Trace

## 边界提醒

`platform` 应该提供能力，不应该主导业务流程。

例如：

- “如何广播事件”属于 `platform`
- “什么时候广播 task.completed”属于 `app`
