# `internal/transport`

这个目录是系统的传输层。

## 作用

传输层负责把外部请求转成应用层调用，再把结果包装回对外协议。

当前包含：

- `http`：REST API
- `ws`：WebSocket 推送

## 当前设计思路

传输层应该尽量“薄”：

- 解析请求
- 做简单校验
- 调用 `internal/app`
- 序列化响应

这样做的好处是：

- 业务逻辑不绑死在 HTTP 上
- 后续接 CLI、gRPC、MQ consumer 更容易
- 测试可以更多地落在应用层，而不是控制器里

## 当前边界

现在的 `transport/http/server.go` 负责：

- 路由注册
- 鉴权中间件
- 请求反序列化
- 响应序列化

`transport/ws/server.go` 负责：

- WebSocket upgrade
- 订阅事件
- 推送文本帧

## 后续演进

后续建议继续拆分：

- DTO 与 handler 分离
- 中间件独立目录
- 路由定义与业务处理解耦
- WebSocket 事件协议标准化
