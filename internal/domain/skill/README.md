# Skill 领域模块说明

这个模块定义平台中的 `Skill` 资源模型。

## 当前产品语义

`Skill` 不再只是“某个工作区里的配置项”，而是平台资源体系的一部分。当前实现支持两类 Skill：

- `platform`
  平台统一提供的 Skill，可被用户安装到自己的可用列表。
- `personal`
  用户自己创建并维护的私有 Skill。

## 主要字段

- 基础标识
  `id`、`workspace_id`、`name`、`slug`
- 资源范围
  `scope`
- 展示信息
  `description`、`version`、`entry`
- 配置结构
  `schema`、`config`
- 生命周期
  `enabled`、`created_by`、`created_at`、`updated_at`

## 当前使用方式

- 平台通过 Skill 注册表统一管理资源
- 用户可以把平台 Skill 安装到“我的 Skill”
- 用户也可以直接创建自己的 `personal skill`
- Agent 通过 `skill_policy` 保存所选 Skill ID 列表

## 说明

当前实现里仍保留了 `workspace_id` 字段用于兼容存储层与历史链路，但前端产品层已经不再把工作区作为核心概念展示。
