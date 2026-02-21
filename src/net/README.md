# Net —— 基于 Reactor 模式的高并发网络库

一个参考 Muduo 设计思想，使用 C++11 实现的 Linux 高性能 TCP 网络库。采用 **多 Reactor 多线程 + 工作线程池** 架构，基于 epoll 边缘触发 (ET) 与非阻塞 IO，支持高并发连接处理。

---

## 目录

- [整体架构](#整体架构)
- [核心组件](#核心组件)
- [线程模型](#线程模型)
- [报文协议](#报文协议)
- [服务端使用方式](#服务端使用方式)
- [客户端使用方式](#客户端使用方式)
- [编译与运行](#编译与运行)
- [文件说明](#文件说明)

---

## 整体架构

### 服务端架构

```
┌──────────────────────────────────────────────────────────────┐
│              业务层 (EchoServer / BankServer / 自定义Server)    │
│     设置回调 → TcpServer，拥有独立工作线程池处理业务逻辑           │
└──────────────────────────┬───────────────────────────────────┘
                           │ 回调链 (std::function + std::bind)
┌──────────────────────────▼───────────────────────────────────┐
│                        TcpServer                              │
│  ┌─────────────┐  ┌───────────────────┐  ┌────────────────┐  │
│  │  mainloop_   │  │  subloops_ (N个)   │  │  IO ThreadPool │  │
│  │  (主Reactor) │  │  (从Reactor)       │  │  ("IO" 线程)   │  │
│  └──────┬───────┘  └────────┬──────────┘  └────────────────┘  │
│         │                   │                                  │
│    ┌────▼────┐     ┌───────▼─────────┐                        │
│    │Acceptor │     │  Connection(s)   │                        │
│    │监听新连接│     │  管理已建立连接    │                        │
│    └─────────┘     └─────────────────┘                        │
└──────────────────────────────────────────────────────────────┘
```

### 客户端架构

```
┌──────────────────────────────────────────────────────────────┐
│                     用户回调 (业务逻辑)                         │
│     连接成功 / 收到消息 / 连接断开 / 错误 / 发送完毕              │
└──────────────────────────┬───────────────────────────────────┘
                           │ 回调链
┌──────────────────────────▼───────────────────────────────────┐
│                        TcpClient                              │
│  ┌─────────────┐  ┌───────────────┐  ┌────────────────────┐  │
│  │  EventLoop   │  │  Connector     │  │  Connection        │  │
│  │  (单线程)    │  │  (发起连接)     │  │  (连接成功后创建)   │  │
│  └──────────────┘  └───────────────┘  └────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 底层组件（服务端/客户端共用）

```
┌──────────────────────────────────────────────────────────────┐
│                        Channel                                │
│           fd + 事件 + 回调 (read/write/close/error)            │
└──────────────────────────┬───────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────────────────────┐
│                         Epoll                                 │
│              epoll_create / epoll_ctl / epoll_wait            │
└──────────────────────────────────────────────────────────────┘
```

---

## 核心组件

| 类 | 职责 |
|---|---|
| **InetAddress** | 封装 `sockaddr_in`，提供 IP/Port 便捷访问 |
| **Socket** | RAII 管理 socket fd，封装 bind/listen/accept，设置非阻塞、地址复用等选项 |
| **Buffer** | 应用层收发缓冲区，处理 TCP 粘包/拆包，支持 4 字节长度前缀协议 |
| **Channel** | 事件通道，绑定 fd + 事件 + 回调函数，是 Epoll 和上层组件之间的桥梁 |
| **Epoll** | 封装 Linux epoll API，管理 Channel 的增删改，返回活跃 Channel 列表 |
| **EventLoop** | Reactor 核心，每个实例运行在一个线程中，包含 Epoll + eventfd 唤醒 + timerfd 定时器 + 任务队列 |
| **Acceptor** | 持有监听 Socket 和对应 Channel，负责接收新连接并通过回调传递给 TcpServer |
| **Connector** | 主动发起非阻塞 TCP 连接，连接成功后通过回调传递 Socket 给 TcpClient（与 Acceptor 对称） |
| **Connection** | 代表一个已建立的 TCP 连接，管理 Socket/Channel/Buffer，使用 `shared_ptr` 管理生命周期 |
| **TcpServer** | 服务端总调度器，组装 mainloop + subloops + Acceptor + IO 线程池，管理所有 Connection |
| **TcpClient** | 客户端总调度器，组装 EventLoop + Connector + Connection，提供异步非阻塞通信 |
| **ThreadPool** | 通用线程池（生产者-消费者模型），用于 IO 线程和工作线程 |
| **Timestamp** | 封装 Unix 时间戳，提供格式化输出 |

---

## 线程模型

采用经典的 **One Loop Per Thread** 模型：

| 角色 | 数量 | 职责 |
|------|------|------|
| **主线程** | 1 | 运行 `mainloop_`，只负责 accept 新连接 |
| **IO 线程** | N（可配置） | 每个线程运行一个 `subloop_`（从 EventLoop），处理已建立连接的 IO 读写 |
| **工作线程** | M（可配置） | 执行业务逻辑（如消息处理、数据库操作等） |

**连接分配**：新连接通过 `fd % N` 分配到从 EventLoop，实现负载均衡。

**跨线程通信**：通过 `EventLoop::queueinloop()` + Linux `eventfd` 唤醒机制，保证线程安全。

---

## 报文协议

网络库默认使用 **4 字节长度前缀** 协议，解决 TCP 粘包/拆包问题：

```
┌──────────────┬──────────────────────────────┐
│  4 字节长度   │        报文内容 (payload)      │
│  (uint32_t)  │                              │
└──────────────┴──────────────────────────────┘
```

Buffer 类的 `sep_` 参数支持三种模式：
- `0`：无分隔符（固定长度，适合视频流等场景）
- `1`：4 字节长度前缀（**默认**，推荐使用）
- `2`：`\r\n\r\n` 分隔符（HTTP 风格，待实现）

---

## 服务端使用方式

### 1. 继承/组合模式创建业务 Server

参考 `EchoServer` 的实现，创建自己的业务服务器只需以下步骤：

```cpp
#include "TcpServer.h"

class MyServer {
private:
    TcpServer tcpserver_;
    ThreadPool threadpool_;  // 工作线程池

public:
    MyServer(const std::string &ip, uint16_t port,
             int subthreadnum = 3, int workthreadnum = 5)
        : tcpserver_(ip, port, subthreadnum),
          threadpool_(workthreadnum, "WORKS")
    {
        // 注册回调函数
        tcpserver_.setnewconnectioncb(
            std::bind(&MyServer::HandleNewConnection, this, std::placeholders::_1));
        tcpserver_.setcloseconnectioncb(
            std::bind(&MyServer::HandleClose, this, std::placeholders::_1));
        tcpserver_.seterrorconnectioncb(
            std::bind(&MyServer::HandleError, this, std::placeholders::_1));
        tcpserver_.setonmessagecb(
            std::bind(&MyServer::HandleMessage, this,
                      std::placeholders::_1, std::placeholders::_2));
        tcpserver_.setsendcompletecb(
            std::bind(&MyServer::HandleSendComplete, this, std::placeholders::_1));
    }

    void Start() { tcpserver_.start(); }
    void Stop()  { tcpserver_.stop(); threadpool_.stop(); }

    // ---- 回调函数 ----
    void HandleNewConnection(spConnection conn) {
        printf("新连接: %s:%d\n", conn->ip().c_str(), conn->port());
    }

    void HandleClose(spConnection conn) {
        printf("连接断开: fd=%d\n", conn->fd());
    }

    void HandleError(spConnection conn) {
        printf("连接错误: fd=%d\n", conn->fd());
    }

    void HandleMessage(spConnection conn, std::string& message) {
        // 将消息处理投递到工作线程池，避免阻塞 IO 线程
        threadpool_.addtask(
            std::bind(&MyServer::OnMessage, this, conn, message));
    }

    void HandleSendComplete(spConnection conn) {
        // 数据发送完成回调
    }

    void OnMessage(spConnection conn, std::string message) {
        // 在工作线程中执行业务逻辑
        std::string reply = "处理结果: " + message;
        conn->send(reply.c_str(), reply.size());
    }
};
```

### 2. 主函数

```cpp
#include <signal.h>

MyServer* server;

void Stop(int sig) {
    server->Stop();
    delete server;
    exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("usage: ./server ip port\n");
        return -1;
    }

    signal(SIGTERM, Stop);
    signal(SIGINT, Stop);

    server = new MyServer(argv[1], atoi(argv[2]), 10, 5);
    server->Start();  // 阻塞运行
}
```

---

## 客户端使用方式

当前客户端（`client.cpp`、`bankclient.cpp`）使用 **原始阻塞 socket** 编写，并未使用 net 网络库。这对于简单客户端场景是够用的。

### 方式一：直接使用阻塞 Socket（当前方式，适合简单场景）

需要手动处理 4 字节报头协议：

```cpp
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// 发送报文（4字节长度前缀）
ssize_t tcpsend(int fd, void* data, size_t size) {
    char tmpbuf[1024];
    memset(tmpbuf, 0, sizeof(tmpbuf));
    memcpy(tmpbuf, &size, 4);           // 前4字节写入报文长度
    memcpy(tmpbuf + 4, data, size);     // 后面写入报文内容
    return send(fd, tmpbuf, size + 4, 0);
}

// 接收报文（4字节长度前缀）
ssize_t tcprecv(int fd, void* data) {
    int len;
    recv(fd, &len, 4, 0);              // 先读4字节长度
    return recv(fd, data, len, 0);     // 再读对应长度的内容
}

int main(int argc, char* argv[]) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(5085);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    // 发送消息
    char buf[1024] = "Hello Server";
    tcpsend(sockfd, buf, strlen(buf));

    // 接收回复
    memset(buf, 0, sizeof(buf));
    tcprecv(sockfd, buf);
    printf("收到回复: %s\n", buf);

    close(sockfd);
}
```

### 方式二：利用 net 库组件构建客户端（推荐，适合复杂场景）

可以复用 net 库中的底层组件（`Socket`、`InetAddress`、`Buffer`）来简化客户端开发：

```cpp
#include "Socket.h"
#include "InetAddress.h"
#include "Buffer.h"
#include <string.h>

int main() {
    // 1. 创建非阻塞 socket
    int fd = createnonblocking();
    Socket sock(fd);

    // 2. 连接服务端
    InetAddress servaddr("127.0.0.1", 5085);
    if (connect(fd, servaddr.addr(), sizeof(sockaddr)) != 0) {
        printf("连接失败\n");
        return -1;
    }

    // 3. 使用 Buffer 组装报文（自动处理4字节报头）
    Buffer sendbuf;
    const char* msg = "Hello Server";
    sendbuf.appendwithsep(msg, strlen(msg));  // 自动添加4字节长度前缀

    // 4. 发送
    send(fd, sendbuf.data(), sendbuf.size(), 0);

    // 5. 接收并拆包
    char tmpbuf[1024];
    int n = recv(fd, tmpbuf, sizeof(tmpbuf), 0);

    Buffer recvbuf;
    recvbuf.append(tmpbuf, n);

    std::string message;
    if (recvbuf.pickmessage(message)) {
        printf("收到回复: %s\n", message.c_str());
    }

    return 0;
}
```

**优势**：使用 `Buffer::appendwithsep()` 自动处理报文封装，`Buffer::pickmessage()` 自动处理拆包，避免手动操作报头字节。

### 方式三：使用 TcpClient 异步客户端（推荐，适合高性能场景）

使用 `TcpClient` 类实现完整的非阻塞异步客户端。`TcpClient` 内部使用 `Connector` 发起非阻塞连接，连接成功后自动创建 `Connection`，复用服务端相同的事件驱动架构。

**核心组件**：
- `Connector`：与 `Acceptor` 对称，主动发起非阻塞 TCP 连接
- `TcpClient`：与 `TcpServer` 对称，管理 EventLoop + Connector + Connection

**回调链**：`Channel → Connection → TcpClient → 用户回调`

```cpp
#include "TcpClient.h"
#include <signal.h>

TcpClient* client;
int msgcount = 0;

void Stop(int sig) {
    client->stop();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("usage: ./echoclient ip port\n");
        return -1;
    }

    signal(SIGTERM, Stop);
    signal(SIGINT, Stop);

    client = new TcpClient(argv[1], atoi(argv[2]));

    // 连接成功回调：发送第一条消息
    client->setconnectioncb([](spConnection conn) {
        printf("连接成功(fd=%d)\n", conn->fd());
        msgcount = 1;
        std::string msg = "hello " + std::to_string(msgcount);
        conn->send(msg.data(), msg.size());
    });

    // 收到消息回调：打印回复，继续发送下一条（ping-pong模式）
    client->setmessagecb([](spConnection conn, std::string& message) {
        printf("收到: %s\n", message.c_str());
        msgcount++;
        if (msgcount <= 10) {
            std::string msg = "hello " + std::to_string(msgcount);
            conn->send(msg.data(), msg.size());
        } else {
            printf("完成所有消息收发\n");
            client->stop();
        }
    });

    // 连接断开 / 错误回调
    client->setclosecb([](spConnection conn) {
        printf("连接断开\n");
    });

    client->seterrorcb([](spConnection conn) {
        printf("连接错误\n");
    });

    client->start();    // 发起连接并启动事件循环（阻塞）

    delete client;
    return 0;
}
```

**TcpClient 工作流程**：

```
TcpClient::start()
    ├── Connector::start()          // 创建非阻塞socket，发起connect
    │       │
    │       ├── connect() 返回 EINPROGRESS
    │       ├── 创建 Channel 监视 EPOLLOUT
    │       │
    │       └── [连接完成] handlewrite()
    │               ├── 检查 SO_ERROR
    │               ├── 创建 Socket
    │               └── 回调 TcpClient::newconnection()
    │                       ├── 创建 Connection
    │                       ├── 设置回调链
    │                       └── 回调用户 connectioncb_
    │
    └── EventLoop::run()            // 进入事件循环
            │
            ├── [收到数据] → Connection::onmessage() → TcpClient::onmessage() → 用户 messagecb_
            ├── [对端关闭] → Connection::closecallback() → TcpClient::closeconnection() → 用户 closecb_
            ├── [发送完毕] → Connection::writecallback() → TcpClient::sendcomplete() → 用户 sendcompletecb_
            └── [stop()调用] → 退出循环
```

**TcpClient 公共接口**：

| 方法 | 说明 |
|------|------|
| `TcpClient(ip, port)` | 构造，指定服务端地址 |
| `start()` | 发起连接并启动事件循环（阻塞直到 `stop()`） |
| `stop()` | 停止事件循环 |
| `send(data, size)` | 发送数据（线程安全，可从任意线程调用） |
| `connection()` | 获取当前 `spConnection` 对象 |
| `setconnectioncb(fn)` | 设置连接成功回调 |
| `setclosecb(fn)` | 设置连接断开回调 |
| `seterrorcb(fn)` | 设置连接错误回调 |
| `setmessagecb(fn)` | 设置收到消息回调 |
| `setsendcompletecb(fn)` | 设置发送完毕回调 |

---

## 编译与运行

### 环境要求

- Linux 操作系统（依赖 epoll、eventfd、timerfd）
- GCC/G++ 支持 C++11
- pthread 库

### 编译

```bash
cd src/net
make            # 编译所有目标
```

### 构建目标

| 目标 | 说明 |
|------|------|
| `tcpepoll` | 回显服务器 (EchoServer) |
| `bankserver` | 网上银行服务器 (BankServer) |
| `client` | 简单回显客户端（阻塞式） |
| `bankclient` | 网上银行客户端（阻塞式） |
| `echoclient` | 异步回显客户端（使用 TcpClient） |

### 运行示例

```bash
# 终端1：启动服务器
./tcpepoll 192.168.232.129 5085

# 终端2：启动客户端（阻塞式）
./client 192.168.232.129 5085

# 或启动异步客户端
./echoclient 192.168.232.129 5085

# 或启动银行示例
./bankserver 192.168.232.129 5085
./bankclient 192.168.232.129 5085
```

### 清理

```bash
make clean
```

---

## 文件说明

```
src/net/
├── InetAddress.h / .cpp    # 网络地址封装
├── Socket.h / .cpp         # Socket fd 封装 (RAII)
├── Buffer.h / .cpp         # 应用层缓冲区 (粘包/拆包处理)
├── Channel.h / .cpp        # 事件通道 (fd + 事件 + 回调)
├── Epoll.h / .cpp          # epoll 封装
├── EventLoop.h / .cpp      # 事件循环 (Reactor核心)
├── Acceptor.h / .cpp       # 新连接接收器（服务端）
├── Connector.h / .cpp      # 连接发起器（客户端）
├── Connection.h / .cpp     # TCP 连接管理（服务端/客户端共用）
├── TcpServer.h / .cpp      # TCP 服务器 (服务端总调度器)
├── TcpClient.h / .cpp      # TCP 客户端 (客户端总调度器)
├── ThreadPool.h / .cpp     # 通用线程池
├── Timestamp.h / .cpp      # 时间戳工具类
├── EchoServer.h / .cpp     # 回显服务器 (业务层示例)
├── BankServer.h            # 银行服务器 (业务层示例)
├── tcpepoll.cpp            # EchoServer 主程序入口
├── bankserver.cpp          # BankServer 主程序入口
├── echoclient.cpp          # 异步回显客户端 (TcpClient示例)
├── client.cpp              # 简单回显客户端（阻塞式）
├── bankclient.cpp          # 银行客户端（阻塞式）
├── makefile                # 构建脚本
└── tmp1.sh                 # 辅助脚本
```

---

## 关键设计要点

1. **多 Reactor 多线程**：主 Reactor 只处理 accept，从 Reactor 处理 IO，工作线程池处理业务逻辑
2. **非阻塞 IO + 边缘触发 (ET)**：所有 fd 使用 `SOCK_NONBLOCK`，Channel 使用 `EPOLLET`
3. **RAII 资源管理**：`Socket` 析构自动关闭 fd，`unique_ptr` 管理 EventLoop/Channel/Socket
4. **跨线程唤醒**：Linux `eventfd` + `queueinloop()` 实现线程安全的任务投递
5. **定时器管理**：`timerfd_create` 创建定时器 fd 纳入 epoll 统一管理，检测连接超时
6. **回调链设计**：`Channel → Connection → TcpServer/TcpClient → 业务层`，逐层 `std::function` 传递
7. **客户端/服务端对称设计**：`Acceptor`(被动接受) 对应 `Connector`(主动连接)，`TcpServer` 对应 `TcpClient`，共用同一个 `Connection` 类


