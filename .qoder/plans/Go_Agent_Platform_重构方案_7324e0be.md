# Go Agent Platform 重构实施方案

## 一、项目定位

**目标**：将现有聊天系统升级为 Agent 应用平台

**核心能力**：
- 多模型切换与流式输出
- 用户级 RAG 知识增强
- Tool/MCP 工具调用
- 异步任务处理
- 多会话上下文隔离

---

## 二、架构设计

### 2.1 整体架构

```
┌─────────────────────────────────────────────────────────┐
│                    Web / 前端                            │
│              HTTP API / WebSocket / SSE                  │
└─────────────────────────────────────────────────────────┘
                           │
┌─────────────────────────────────────────────────────────┐
│                  Go Agent Server                         │
├───────────┬───────────┬───────────┬─────────────────────┤
│ 用户会话层 │ 模型调度层 │  RAG层    │    Tool/MCP层       │
├───────────┼───────────┼───────────┼─────────────────────┤
│ MySQL     │ OpenAI    │ Qdrant    │ MCP Client          │
│ Redis     │ DeepSeek  │ Embedding │ 内置工具集           │
│ RabbitMQ  │ Ollama    │ 服务      │ weather/search/...  │
└───────────┴───────────┴───────────┴─────────────────────┘
                           │
┌─────────────────────────────────────────────────────────┐
│              Worker (异步任务处理)                        │
│      文件解析 / 向量化 / 消息持久化 / TTS                  │
└─────────────────────────────────────────────────────────┘
```

### 2.2 目录结构

```
go-agent-platform/
├── cmd/
│   ├── api/                    # API服务入口
│   └── worker/                 # 后台任务Worker入口
├── internal/
│   ├── domain/                 # 领域层
│   │   ├── agent/              # Agent编排
│   │   ├── auth/               # 认证授权
│   │   ├── session/            # 会话管理
│   │   ├── task/               # 任务管理
│   │   ├── tool/               # 工具定义
│   │   └── workspace/          # 工作空间
│   ├── platform/               # 平台能力层
│   │   ├── llm/                # LLM模型抽象
│   │   ├── memory/             # 记忆存储
│   │   ├── events/             # 事件总线
│   │   └── scheduler/          # 任务调度
│   ├── transport/              # 传输层
│   │   ├── http/               # HTTP API
│   │   └── ws/                 # WebSocket
│   ├── config/                 # 配置管理
│   └── app/                    # 应用组装
├── migrations/                 # 数据库迁移
├── configs/                    # 配置文件
└── deployments/                # 部署配置
```

---

## 三、技术选型

| 层级 | 技术栈 | 说明 |
|------|--------|------|
| HTTP框架 | Gin | REST API |
| 实时通信 | gorilla/websocket + SSE | 双工聊天 + 流式输出 |
| 主数据库 | MySQL | 业务数据持久化 |
| 缓存 | Redis | Session、缓存、临时上下文 |
| 消息队列 | RabbitMQ | 异步任务队列 |
| 向量库 | Qdrant | RAG向量存储 |
| 配置管理 | Viper | 配置热加载 |
| 日志 | Zap | 结构化日志 |
| 链路追踪 | OpenTelemetry | 可观测性 |

---

## 四、核心接口设计

### 4.1 模型抽象接口

```go
// internal/platform/llm/model.go
type AIModel interface {
    Chat(ctx context.Context, req ChatRequest) (ChatResponse, error)
    StreamChat(ctx context.Context, req ChatRequest) (<-chan StreamEvent, error)
    Embedding(ctx context.Context, texts []string) ([][]float32, error)
}

// 工厂方法
func NewModel(provider string, cfg Config) AIModel
```

### 4.2 工具抽象接口

```go
// internal/domain/tool/tool.go
type Tool interface {
    Name() string
    Description() string
    Invoke(ctx context.Context, args map[string]any) (string, error)
}
```

### 4.3 RAG Pipeline接口

```go
// internal/platform/rag/pipeline.go
type RAGPipeline interface {
    Index(ctx context.Context, doc Document) error
    Retrieve(ctx context.Context, query string, opts RetrieveOptions) ([]Chunk, error)
}
```

---

## 五、数据库设计

### 5.1 现有表（保留）

- users
- personas
- conversations
- messages

### 5.2 新增表

**文件/RAG模块**：
- files - 文件元数据
- file_parse_tasks - 文件解析任务
- knowledge_chunks - 知识切片
- retrieval_logs - 检索日志

**Agent模块**：
- tool_call_logs - 工具调用日志
- session_memories - 会话记忆
- model_call_logs - 模型调用日志

**异步任务模块**：
- job_records - 任务记录
- dead_letter_logs - 死信日志

---

## 六、实施阶段

### 阶段1：Go版底座（第1-2周）

**目标**：完成基础聊天功能迁移

**任务清单**：
- [ ] 用户/会话/消息 REST API
- [ ] WebSocket 双向通信
- [ ] SSE 流式输出
- [ ] 多模型抽象接口
- [ ] 会话上下文管理模块

**产出**：Go版多会话AI聊天系统

---

### 阶段2：多模型能力（第3-4周）

**目标**：实现模型层可插拔

**任务清单**：
- [ ] DeepSeek 适配器实现
- [ ] OpenAI 适配器实现
- [ ] Ollama 本地模型适配器
- [ ] 模型配置热加载
- [ ] 流式输出统一封装

**产出**：支持多模型切换的聊天系统

---

### 阶段3：RAG最小闭环（第5-6周）

**目标**：实现知识增强能力

**任务清单**：
- [ ] 文件上传接口
- [ ] 文件解析服务（PDF/TXT/MD）
- [ ] 文本切块策略
- [ ] Embedding 服务对接
- [ ] Qdrant 向量库集成
- [ ] 检索增强回答流程

**产出**：具备知识库问答能力

---

### 阶段4：异步任务系统（第7-8周）

**目标**：解耦耗时操作

**任务清单**：
- [ ] RabbitMQ 集成
- [ ] 文档处理队列（file.parse/file.embed/file.index）
- [ ] 聊天异步队列（message.persist/conversation.summary）
- [ ] Worker 消费者实现
- [ ] 任务状态追踪

**产出**：异步化文档处理和消息持久化

---

### 阶段5：Tool/MCP能力（第9-10周）

**目标**：从聊天系统升级为Agent平台

**任务清单**：
- [ ] Tool 抽象接口
- [ ] 内置工具实现（weather/time/calculator）
- [ ] 工具调用决策逻辑
- [ ] Tool Call 日志记录
- [ ] MCP 协议适配（可选）

**产出**：具备工具调用能力的Agent系统

---

### 阶段6：工程化完善（第11-12周）

**目标**：生产级部署能力

**任务清单**：
- [ ] Docker Compose 一键部署
- [ ] 日志/监控集成
- [ ] 配置驱动化
- [ ] 错误码统一
- [ ] API文档生成
- [ ] 性能优化

**产出**：可演示、可部署的完整平台

---

## 七、上下文三层架构

最终 Prompt 拼装结构：

```
┌─────────────────────────────────────┐
│ 1. System Prompt                     │
│    - 角色定义、行为约束               │
├─────────────────────────────────────┤
│ 2. Persona Prompt                    │
│    - 人设信息、个性设定               │
├─────────────────────────────────────┤
│ 3. Session History (短期记忆)         │
│    - 最近N轮对话                      │
├─────────────────────────────────────┤
│ 4. Retrieved Docs (外部知识)          │
│    - RAG检索结果                      │
├─────────────────────────────────────┤
│ 5. Tool Results (工具结果)            │
│    - 工具调用返回                     │
├─────────────────────────────────────┤
│ 6. Safety Constraints                 │
│    - 输出格式、安全约束               │
└─────────────────────────────────────┘
```

---

## 八、优先级排序

| 优先级 | 模块 | 原因 |
|--------|------|------|
| P0 | Go化底座 + 多模型抽象 | 一切能力的基础 |
| P1 | RAG最小闭环 | 核心差异化能力 |
| P2 | Tool/MCP + RabbitMQ | Agent能力关键 |

---

## 九、风险控制

1. **渐进式重构**：不破坏现有功能，逐步叠加新能力
2. **接口先行**：先定义接口，再实现，便于替换
3. **配置驱动**：关键参数可配置，避免硬编码
4. **可观测性**：日志、监控、链路追踪尽早接入