# 数据库迁移说明

这个目录存放平台数据库迁移文件。

## 当前文件

- [001_init.sql](C:\Users\26594\Desktop\项目文件\go-agent-platform\migrations\001_init.sql)

这份迁移负责初始化首版平台的核心表：

- `users`
- `auth_sessions`
- `workspaces`
- `memberships`
- `agents`
- `agent_versions`
- `tools`
- `sessions`
- `messages`
- `tasks`
- `task_steps`
- `approvals`
- `schedules`
- `audit_events`

## 当前执行方式

当：

- `STORAGE_DRIVER=postgres`
- `POSTGRES_AUTO_MIGRATE=true`

时，[internal/platform/postgres/store.go](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\platform\postgres\store.go) 会在应用启动时自动执行迁移。

## 当前特点

- 已使用 `CREATE TABLE IF NOT EXISTS`
- 允许开发环境和测试环境重复执行
- 可以配合 PostgreSQL 集成测试自动建表

## 后续建议

1. 拆分成多版本 migration 文件
2. 单独维护索引和字段变更脚本
3. 在生产环境中接入专门的迁移工具
