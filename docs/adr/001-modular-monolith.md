# ADR-001: Modular Monolith for V1

## Status

Accepted

## Context

项目需要从单一聊天系统重构为多 Agent 管理平台，但当前仓库尚无 Go 运行时、数据库驱动或服务拆分基础。直接进入微服务会显著提高交付与调试成本。

## Decision

首版采用 Go 模块化单体：

- 以领域包边界组织代码，而不是按技术层零散堆放
- 统一进程内完成 API、编排、审批、会话与调度的主链路
- 对存储、LLM、事件推送保留接口边界，方便后续拆分

## Consequences

- 优点：实现速度快，调试路径短，适合从旧系统迁移到新平台
- 缺点：worker 与 api 的最终解耦仍需后续补充持久化和队列实现
- 演进路径：先把 `memory` 存储替换为 PostgreSQL/Redis，再拆出独立 worker
