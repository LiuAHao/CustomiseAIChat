# 基于高并发服务器的AI聊天系统框架设计

## 系统架构
```plaintext
[Qt客户端] <--WebSocket/HTTP--> [高并发服务器] <--API调用--> [AI服务(如OpenAI API)]
 ```
```

## 核心组件设计
### 1. 服务端处理
```cpp
class AIServer : public TcpServer {
private:
    std::map<std::string, Persona> personas_; // 人设存储(key: persona_id)
    std::map<int, std::string> client_persona_; // 客户端与人设映射
    
public:
    void HandleMessage(spConnection conn, std::string message) override;
    void AddPersona(const Persona& persona);
    void RemovePersona(const std::string& persona_id);
};
 ```
```

### 2. 消息协议设计（建议JSON格式）
```json
// 客户端->服务端
{
    "type": "chat|persona_create|persona_select",
    "persona_id": "xxx", // 人设ID
    "content": "你好",   // 聊天内容
    "persona_config": {  // 人设配置(创建时)
        "name": "学霸助手",
        "prompt": "你是一个严谨的学术助手...",
        "avatar": "base64数据"
    }
}

// 服务端->客户端
{
    "type": "chat_response|persona_list",
    "persona_id": "xxx",
    "content": "你好，我是学霸助手...",
    "personas": [ // 人设列表
        {"id": "p1", "name": "学霸助手", "avatar": "..."},
        {...}
    ]
}
 ```
```

### 3. Qt前端框架
```plaintext
MainWindow
├── PersonaListWidget (左侧人设列表)
│   ├── PersonaItemWidget (单个人设项)
│   └── AddPersonaDialog (添加人设对话框)
└── ChatAreaWidget (右侧聊天区域)
    ├── MessageDisplayWidget (消息显示)
    └── MessageInputWidget (输入框+发送按钮)
 ```
```

## 数据流处理流程
1. 人设创建 ：
   
   - 用户在Qt前端填写人设信息
   - 前端发送 persona_create 消息到服务端
   - 服务端持久化存储人设(建议SQLite/文件存储)
2. 聊天会话 ：
   
   - 用户选择人设后开始聊天
   - 前端发送 chat 消息，附带 persona_id 和 content
   - 服务端根据 persona_id 获取预设prompt，组合成完整提示词调用AI服务
   - 返回AI响应给对应客户端
3. 人设管理 ：
   
   - 服务端启动时加载所有已保存人设
   - 提供人设列表查询接口
## 存储设计建议
1. 人设存储 ：
   
   ```plaintext
   /personas/
   ├── p1.json
   ├── p2.json
   └── ...
    ```
2. 单个人设文件结构 ：
   
   ```json
   {
       "id": "p1",
       "name": "学霸助手",
       "prompt": "你是一个严谨的学术助手...",
       "avatar": "base64数据",
       "create_time": "2023-01-01"
   }
    ```


###数据库设计
使用SQLite数据库

数据库表结构设计
### 主数据库文件：chat.db
```sql
-- 人设表
CREATE TABLE IF NOT EXISTS personas (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    prompt TEXT NOT NULL,
    avatar TEXT,
    create_time DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 聊天会话表
CREATE TABLE IF NOT EXISTS conversations (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    persona_id TEXT NOT NULL,
    title TEXT NOT NULL,
    create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (persona_id) REFERENCES personas(id)
);

-- 聊天消息表
CREATE TABLE IF NOT EXISTS messages (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    conversation_id INTEGER NOT NULL,
    sender_type INTEGER NOT NULL, -- 0:用户 1:AI
    content TEXT NOT NULL,
    send_time DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (conversation_id) REFERENCES conversations(id)
);
 ```
```

## 3. 服务端数据库操作封装
在服务端添加一个数据库管理类：

```cpp
class DatabaseManager {
public:
    static DatabaseManager& instance();
    
    bool init(const std::string& dbPath);
    
    // 人设操作
    bool addPersona(const Persona& persona);
    bool removePersona(const std::string& personaId);
    std::vector<Persona> getAllPersonas();
    
    // 会话操作
    int createConversation(const std::string& personaId, const std::string& title);
    std::vector<Conversation> getConversations(const std::string& personaId);
    
    // 消息操作
    bool addMessage(int conversationId, int senderType, const std::string& content);
    std::vector<Message> getMessages(int conversationId);
    
private:
    DatabaseManager();
    sqlite3* db_;
};
 ```
```

## 4. 消息历史查询协议
在原有消息协议基础上扩展：

```json
// 客户端请求历史消息
{
    "type": "history_request",
    "persona_id": "xxx",
    "conversation_id": 123  // 可选，不传则获取最新会话
}

// 服务端返回历史消息
{
    "type": "history_response",
    "persona_id": "xxx",
    "conversations": [
        {
            "id": 123,
            "title": "第一次对话",
            "create_time": "2023-01-01"
        }
    ],
    "messages": [
        {
            "sender_type": 0,
            "content": "你好",
            "send_time": "2023-01-01 10:00"
        },
        {
            "sender_type": 1,
            "content": "你好，我是AI助手",
            "send_time": "2023-01-01 10:01"
        }
    ]
}
 ```
```

## 5. Qt前端历史消息显示
建议在聊天区域上方添加会话历史面板：

```plaintext
MainWindow
├── PersonaListWidget (左侧人设列表)
└── ChatAreaWidget (右侧聊天区域)
    ├── ConversationHistoryWidget (会话历史列表)
    ├── MessageDisplayWidget (消息显示)
    └── MessageInputWidget (输入框+发送按钮)
 ```
```

## 6. 初始化数据库
服务端启动时应初始化数据库：

```cpp
bool AIServer::Start() {
    // ... 原有代码 ...
    
    // 初始化数据库
    if (!DatabaseManager::instance().init("chat.db")) {
        LOG_ERROR << "Failed to initialize database";
        return false;
    }
    
    // 加载所有人设
    personas_ = DatabaseManager::instance().getAllPersonas();
    
    // ... 原有代码 ...
}
 ```
```


###python接口调用设计

1. 核心架构
```plaintext
[Flask服务] <--HTTP--> [DeepSeek API]

2. 接口规范
核心API端点 ：

- POST /api/v1/chat - 主聊天接口（供C++服务器调用）
- GET /test - 测试页面
- POST /test/api/chat - 测试接口
请求/响应格式 ：

```json
// 请求示例
{
    "persona_id": "p1",
    "messages": [
        {"role": "user", "content": "你好"}
    ]
}

// 响应示例
{
    "success": true,
    "response": "你好，我是AI助手..."
}
 ```
```
 3. 关键技术实现
- 多语言支持 ：自动处理中英文编码（UTF-8）
- 人设系统 ：通过 persona_id 加载不同预设prompt
- 错误处理 ：统一JSON错误响应格式
- 测试界面 ：提供可视化聊天测试页面 4. 部署要求
```bash
# 依赖安装
pip install flask openai
 ```
 5. 扩展接口
```python
# 添加新人设
def add_persona(persona_id, name, prompt, avatar=None):
    personas[persona_id] = {
        "name": name,
        "prompt": prompt,
        "avatar": avatar
    }
 ```
```

使用方法：

1. 核心API：C++服务器继续调用 http://localhost:5000/api/v1/chat
2. 测试页面：访问 http://localhost:5000/test