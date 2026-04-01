# PostgreSQL 存储模块

这个模块负责平台的 PostgreSQL 持久化实现，当前承担三类职责：

- 建立数据库连接池
- 执行基础迁移与种子数据初始化
- 实现 `internal/app.Store` 所需的全量持久化方法

## 目录说明

- `store.go`
  PostgreSQL 存储实现，包括连接、迁移、种子数据与 CRUD 方法。

## 设计约束

- 首版仍是模块化单体，因此这里直接实现应用层需要的存储接口，不提前拆成过细的 repository 包。
- 对外暴露的是 `app.Store` 抽象，业务层不应直接引用 SQL 细节。
- JSON 字段统一使用 `JSONB`，对应 Go 中的 `map[string]any` / `[]tool.CallSpec`。

## 后续可继续演进

- 将 `store.go` 再拆分为 `auth/agent/task/...` 子仓储
- 接入迁移工具，如 `golang-migrate`
- 为复杂链路补事务封装
