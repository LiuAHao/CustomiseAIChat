# Model 领域模块说明

这个模块定义平台中的模型注册表资源。

## 当前产品语义

模型用于：

- Agent 创建时选择默认模型
- Agent 聊天页中按需切换执行模型
- 为任务执行记录 `model` 和 `reasoning` 维度

当前实现中，模型仍是平台注册资源，还没有进一步拆成“平台模型 / 我的模型”两层结构。

## 主要字段

- 标识与来源
  `id`、`workspace_id`、`provider`、`model_key`
- 展示信息
  `name`、`description`
- 能力边界
  `context_window`、`max_output_tokens`、`capabilities`
- 生命周期
  `enabled`、`is_default`、`created_by`、`created_at`、`updated_at`

## 当前使用方式

- Agent 的 `model` 字段保存 `model_key`
- 前端模型配置页直接管理模型注册表
- 对话执行时允许按请求覆盖默认模型

## 说明

当前同样保留了 `workspace_id` 用于兼容后端现有链路，但前端已经不把工作区作为产品概念暴露。
