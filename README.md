# CustomiseAIChat - 自定义人设AI聊天系统

## 目录
1. [系统架构](#系统架构)
2. [核心功能](#核心功能)
3. [技术栈](#技术栈)
4. [项目结构](#项目结构)
5. [数据库设计](#数据库设计)
6. [消息协议](#消息协议)
7. [部署指南](#部署指南)
8. [前端说明](#前端说明)

---

## 系统架构

```
┌─────────────┐   WebSocket   ┌──────────────────┐   TCP (4B头协议)  ┌──────────────┐
│   Web前端   │ ────────────► │  ws_proxy.py     │ ────────────────► │  C++后端服务  │
│ (浏览器)    │               │  (WS+HTTP代理)   │                   │ (ChatServer)  │
└─────────────┘               └──────────────────┘                   └──────┬───────┘
      ↑                                                                      │
      │ HTTP :8080                                                            ├── SQLite3
      └──────────────────────────────────────────────────────────────────────┤
                                                                              │  HTTP/JSON
                                                                              └──► DeepSeek API
```

**端口说明**

| 端口 | 服务 |
|------|------|
| `8080` | HTTP 静态文件服务（前端页面） |
| `8765` | WebSocket 代理（前端 ↔ 后端桥接） |
| `5085` | C++ TCP 后端（仅内部通信） |

---

## 核心功能

- **用户系统**：注册、登录、会话 Token 认证、昵称/密码管理、自定义头像、账号注销
- **人设管理**：创建/编辑/删除 AI 人设，支持表情或自定义图片头像、描述、系统提示词
- **AI 一键生成描述**：输入人设名称，AI 自动生成角色描述
- **AI 主动打招呼**：新建会话时人设主动发起问候，沉浸式体验
- **多会话管理**：每个人设下可独立维护多个会话，上下文互不干扰
- **实时聊天**：基于 WebSocket 的全双工通信，AI 回复自动携带历史上下文
- **聊天记录**：持久化存储于 SQLite3，支持查询与清空
- **高并发后端**：自研 Epoll Reactor 网络库 + 工作线程池
- **安全配置**：API 密钥通过 `.env` 文件注入，不入 Git

---

## 技术栈

| 层级 | 技术 |
|------|------|
| 网络层 | 自研 TCP 库（Epoll + 非阻塞 IO + Reactor 模式） |
| 业务层 | C++17，工作线程池异步处理 |
| 数据库 | SQLite3，WAL 模式，外键约束级联删除 |
| AI 服务 | DeepSeek API（libcurl HTTP 调用） |
| 序列化 | JsonCpp |
| 安全 | OpenSSL（SHA-256 密码哈希 + 随机盐值） |
| 构建 | CMake 3.10+ |
| 代理服务 | Python3 + websockets（WebSocket ↔ TCP 桥接 + 静态文件） |
| 前端 | HTML5 / Tailwind CSS CDN / 原生 JavaScript |
| Qt 客户端 | Qt 5.15+（可选构建） |

---

## 项目结构

```
CustomiseAIChat/
├── CMakeLists.txt              # 顶层构建配置
├── .env.example                # 环境变量示例（复制为 data/.env 填入密钥）
├── data/
│   ├── server.conf             # 服务器主配置（提交到 Git）
│   └── .env                    # 私密配置：ai_api_key（已 .gitignore）
├── build/                      # CMake 构建输出目录
└── src/
    ├── net/                    # 自研网络库
    │   ├── Acceptor.*          # 连接接受器
    │   ├── Buffer.*            # 收发缓冲区
    │   ├── Channel.*           # 事件通道
    │   ├── Connection.*        # TCP 连接封装
    │   ├── Epoll.*             # Epoll 封装
    │   ├── EventLoop.*         # 事件循环（主/子 Reactor）
    │   ├── InetAddress.*       # 网络地址封装
    │   ├── Socket.*            # Socket 封装
    │   ├── TcpServer.*         # TCP 服务端
    │   ├── TcpClient.*         # TCP 客户端
    │   ├── ThreadPool.*        # 工作线程池
    │   └── Timestamp.*         # 时间戳工具
    ├── server/                 # 业务后端
    │   ├── main.cpp            # 入口：初始化、信号处理、配置加载
    │   ├── ChatServer.*        # 请求解析与 Action 路由
    │   ├── HttpClient.*        # libcurl HTTP 客户端（调用 AI API）
    │   ├── config/
    │   │   └── ServerConfig.*  # 配置文件解析（server.conf + .env）
    │   ├── db/
    │   │   └── Database.*      # SQLite3 数据库操作层
    │   ├── model/              # 数据模型（纯结构体）
    │   │   ├── User.h
    │   │   ├── Persona.h
    │   │   ├── Conversation.h
    │   │   └── Message.h
    │   └── service/            # 业务逻辑层
    │       ├── UserService.*   # 用户注册/登录/认证/Token 管理
    │       ├── ChatService.*   # 人设管理/会话管理/历史记录
    │       └── AIService.*     # AI 消息发送、主动问候、描述生成
    ├── ui/                     # Qt 客户端（可选）
    └── web/                    # Web 前端
        ├── index.html          # 单页应用入口
        ├── ws_proxy.py         # 前端一体化代理服务器（HTTP + WebSocket）
        ├── css/
        │   └── style.css
        └── js/
            ├── api.js          # 后端 API 封装（FIFO 请求队列）
            └── app.js          # 应用逻辑（视图、状态、交互）
```

---

## 数据库设计

### ER 关系

```
User (1) ──→ (N) Persona (1) ──→ (N) Conversation (1) ──→ (N) Message
  │                                          ↑
  └──────────────────────────────────────────┘
              (user_id 双重归属，保证权限校验)
```

- 删除用户：级联删除其所有人设、会话、消息
- 删除人设：级联删除其所有会话和消息
- 同一人设下可维护多个独立会话，互不共享上下文

### 表结构

```sql
-- 用户表
CREATE TABLE users (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    username      TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    salt          TEXT NOT NULL,
    nickname      TEXT NOT NULL,
    created_at    DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at    DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 人设表
CREATE TABLE personas (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id       INTEGER NOT NULL,
    name          TEXT NOT NULL,
    description   TEXT DEFAULT '',    -- 展示描述
    system_prompt TEXT DEFAULT '',    -- 发送给 AI 的系统提示词
    avatar        TEXT DEFAULT '',    -- 'emoji:🤖' 或 Base64 Data URL
    created_at    DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at    DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- 会话表
CREATE TABLE conversations (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id       INTEGER NOT NULL,
    persona_id    INTEGER NOT NULL,
    title         TEXT DEFAULT '新会话',
    message_count INTEGER DEFAULT 0,
    created_at    DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at    DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(user_id)    REFERENCES users(id)    ON DELETE CASCADE,
    FOREIGN KEY(persona_id) REFERENCES personas(id) ON DELETE CASCADE
);

-- 消息表
CREATE TABLE messages (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    conversation_id INTEGER NOT NULL,
    role            TEXT NOT NULL,    -- 'user' / 'assistant'
    content         TEXT NOT NULL,
    timestamp       DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(conversation_id) REFERENCES conversations(id) ON DELETE CASCADE
);
```

### 索引

```sql
CREATE INDEX idx_personas_user         ON personas(user_id);
CREATE INDEX idx_conversations_user    ON conversations(user_id);
CREATE INDEX idx_conversations_persona ON conversations(persona_id);
CREATE INDEX idx_conversations_updated ON conversations(updated_at);
CREATE INDEX idx_messages_conversation ON messages(conversation_id);
CREATE INDEX idx_messages_timestamp    ON messages(timestamp);
```

---

## 消息协议

所有通信均使用 **JSON** 格式。TCP 层采用 **4 字节小端长度头 + Body** 的帧协议；Web 前端通过 WebSocket 与代理交互，格式相同。每条请求必须包含 `action` 字段，需要鉴权的请求须附带 `token` 字段。

### 通用响应格式

```json
{ "code": 0, "message": "操作成功" }
```

| code | 含义 |
|------|------|
| `0` | 成功 |
| `-1` | 业务错误 |
| `-2` | 未登录或会话已过期 |

### API 列表

#### 用户管理（无需 Token）

| Action | 参数 | 返回字段 |
|--------|------|---------|
| `register` | `username`, `password`, `nickname` | `userId` |
| `login` | `username`, `password` | `token`, `userId`, `nickname` |

#### 用户管理（需要 Token）

| Action | 参数 | 说明 |
|--------|------|------|
| `logout` | — | 使当前 Token 失效 |
| `get_user_info` | — | 返回 `userId`, `username`, `nickname` |
| `update_nickname` | `nickname` | 修改昵称 |
| `update_password` | `oldPassword`, `newPassword` | 修改密码 |
| `delete_account` | `password` | 注销账号（级联删除所有数据） |

#### 人设管理（需要 Token）

| Action | 参数 | 说明 |
|--------|------|------|
| `create_persona` | `name`, `description`, `systemPrompt`, `avatar` | 创建人设，返回 `personaId` |
| `update_persona` | `personaId`, `name`, `description`, `systemPrompt`, `avatar` | 更新人设 |
| `delete_persona` | `personaId` | 删除人设（级联删除会话和消息） |
| `list_personas` | — | 返回当前用户所有人设列表 |
| `get_persona` | `personaId` | 返回人设详情 |
| `generate_desc` | `name` | AI 根据人设名称生成描述，返回 `description` |

#### 会话管理（需要 Token）

| Action | 参数 | 说明 |
|--------|------|------|
| `create_conversation` | `personaId`, `title`（可选） | 创建会话，返回 `conversationId` |
| `delete_conversation` | `conversationId` | 删除会话及其所有消息 |
| `list_conversations` | `limit`（可选，默认 50） | 获取用户所有会话（按更新时间倒序） |
| `list_conversations_by_persona` | `personaId` | 获取指定人设下的所有会话 |
| `get_conversation` | `conversationId` | 获取会话详情 |
| `update_conversation_title` | `conversationId`, `title` | 修改会话标题 |

#### 聊天操作（需要 Token）

| Action | 参数 | 说明 |
|--------|------|------|
| `send_message` | `conversationId`, `message` | 发送消息，返回 `reply`（AI 回复）、`timestamp` |
| `greet` | `conversationId` | 让人设主动打招呼，返回 `reply`、`timestamp` |
| `get_history` | `conversationId`, `limit`（可选） | 获取聊天记录，返回 `messages` 数组 |
| `clear_history` | `conversationId` | 清空会话聊天记录 |

### 请求/响应示例

**发送消息：**
```json
// 请求
{
    "action": "send_message",
    "token": "abc123...",
    "conversationId": 5,
    "message": "你好，请介绍一下自己"
}

// 响应
{
    "code": 0,
    "reply": "你好！我是……",
    "timestamp": "2026-02-25T12:00:00Z"
}
```

**AI 生成人设描述：**
```json
// 请求
{ "action": "generate_desc", "token": "abc123...", "name": "星际探险家" }

// 响应
{ "code": 0, "description": "热爱星辰大海，擅长宇宙知识问答……" }
```

---

## 部署指南

### 环境依赖

**后端（Linux，需支持 Epoll）：**
- GCC 7+ / Clang 5+（C++17）
- CMake 3.10+
- `libsqlite3-dev`
- `libjsoncpp-dev`
- `libcurl4-openssl-dev`
- `libssl-dev`

**前端代理：**
- Python3
- `websockets` 库

### 编译后端

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 配置

**1. 主配置文件 `data/server.conf`（可提交到 Git）：**

```properties
server_ip=0.0.0.0
server_port=5085
io_threads=3
work_threads=5
db_path=./data/chat.db

ai_api_url=https://api.deepseek.com/v1/chat/completions
ai_model=deepseek-chat
ai_temperature=0.7
ai_max_tokens=2048
max_history_context=10
```

**2. 私密配置 `data/.env`（已被 `.gitignore` 忽略，需手动创建）：**

```properties
ai_api_key=sk-your-api-key-here
```

> 可参考项目根目录的 `.env.example` 模板。`.env` 加载优先级高于 `server.conf`，同名 key 以 `.env` 为准。

### 启动后端

```bash
# 在项目根目录执行
./build/src/server/chat_server ./data/server.conf
```

### 启动前端代理

```bash
# 安装依赖（仅首次）
pip3 install websockets

# 启动（在项目根目录执行，同时提供 HTTP:8080 和 WS:8765）
python3 src/web/ws_proxy.py
```

启动后访问 [http://localhost:8080](http://localhost:8080) 即可使用 Web 界面。

### 构建 Qt 客户端（可选）

```bash
cmake .. -DBUILD_CLIENT=ON
make -j$(nproc)
```

---

## 前端说明

### Web 版

Web 前端位于 `src/web/`，使用 **Tailwind CSS + 原生 JavaScript** 构建，无需打包工具。

通过 `ws_proxy.py` 实现前后端桥接：
- **HTTP `:8080`**：托管 `src/web/` 下的静态文件
- **WebSocket `:8765`**：接收前端 WebSocket 消息，转发到 C++ TCP 后端，返回响应

前端功能包括：
- 登录/注册，支持"记住我"（localStorage 持久化会话）
- 人设列表、创建/编辑/删除人设（支持 Emoji 或自定义上传图片头像）
- 每个人设自动创建会话并触发 AI 主动问候
- 打字机风格的 AI 思考动效
- 设置面板：修改昵称、密码、上传用户头像、查看订阅等级

### Qt 版

Qt 客户端位于 `src/ui/`，需开启 `-DBUILD_CLIENT=ON` 编译，直接通过 TCP 与后端通信。
