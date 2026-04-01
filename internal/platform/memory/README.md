# Memory 存储模块

这个模块提供平台的内存存储实现，主要用于以下场景：

- 本地快速开发
- 单元测试
- 默认 HTTP 集成测试

## 模块职责

- 实现 [internal/app/store.go](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\app\store.go) 定义的统一存储接口
- 提供默认管理员和默认工作区的种子初始化
- 用 map 结构模拟平台各领域对象的持久化行为

## 当前覆盖对象

- 用户与登录态
- 工作区与成员关系
- Agent / Version
- Tool
- Session / Message
- Task / TaskStep
- Approval
- Schedule
- AuditEvent

## 定位说明

`memory` 不是生产持久化方案，它的主要价值是让应用层在没有外部数据库时也能快速回归。

它和 [internal/platform/postgres](C:\Users\26594\Desktop\项目文件\go-agent-platform\internal\platform\postgres) 一样，都实现同一套 `app.Store`。业务层不应该依赖这里的具体 map 结构。
