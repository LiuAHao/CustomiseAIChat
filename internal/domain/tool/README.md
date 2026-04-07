# Tool / MCP 领域模块说明

这个模块定义平台中的 `Tool` 资源模型，前端产品层将其展示为 `MCP`。

## 当前产品语义

`Tool` 支持两类范围：

- `platform`
  平台统一提供的 MCP，用户可安装到“我的 MCP”。
- `personal`
  用户自己创建并维护的私有 MCP。

## 主要字段

- 基础标识
  `id`、`workspace_id`、`name`
- 资源范围
  `scope`
- 展示信息
  `description`
- 结构配置
  `schema`、`config`
- 调用约束
  `requires_approval`
- 生命周期
  `enabled`、`created_by`、`created_at`

## 当前使用方式

- 平台通过 MCP 注册表统一管理资源
- 用户可以安装平台 MCP 到“我的 MCP”
- 用户也可以创建自己的私有 MCP
- Agent 通过 `tool_policy` 保存所选 MCP ID 列表
- 任务执行时根据 MCP 配置和审批要求决定是否直接调用

## 说明

当前后端领域仍使用 `Tool` 作为命名，以兼容已有执行链路；前端与产品层统一展示为 `MCP`。
