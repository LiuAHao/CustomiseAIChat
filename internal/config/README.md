# `internal/config`

这个目录负责配置读取。

## 作用

- 统一管理环境变量
- 提供默认值
- 向上层输出强类型配置对象

## 当前文件

- `config.go`

## 当前配置项

当前主要包括：

- `APP_NAME`
- `HTTP_ADDR`
- `WORKER_POLL_INTERVAL`
- `JWT_SECRET`
- `SEED_ADMIN_EMAIL`
- `SEED_ADMIN_PASSWORD`
- `STORAGE_DRIVER`

## 设计原则

配置读取应该尽量简单：

- 只做读取与默认值处理
- 不夹带业务逻辑
- 不依赖传输层

## 后续演进

后续可以增加：

- PostgreSQL 配置
- Redis 配置
- LLM Provider 配置
- 对象存储配置
- 日志级别、Trace、Metrics 配置

如果配置复杂度再提升，可以考虑：

- 分文件配置结构体
- 配置校验
- 环境分层
