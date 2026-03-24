# `cmd/api`

这个目录是后端 API 进程入口。

## 作用

- 启动 HTTP 服务
- 暴露 REST API
- 暴露 `/ws` WebSocket 事件流
- 在当前版本中顺带轮询调度器，触发到期任务

## 当前文件

- `main.go`

## 启动流程

1. 从 `internal/config` 读取配置
2. 用 `internal/app` 创建应用实例
3. 用 `internal/transport/http` 创建 HTTP Server
4. 启动 API 服务
5. 启动一个定时轮询协程，调用 `RunDueSchedules()`

## 为什么放在 `cmd`

`cmd` 目录下的代码只负责“把系统跑起来”，不负责业务细节。业务逻辑应尽量下沉到 `internal` 中，避免入口文件膨胀。

## 后续演进

后续如果把调度和执行从 API 进程中彻底拆出，这里会只保留：

- API 启动
- 中间件注册
- 优雅退出
- 观测初始化
