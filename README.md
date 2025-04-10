# AI聊天系统技术文档

## 目录
1. [系统架构](#系统架构)
2. [核心功能](#核心功能)
3. [技术栈](#技术栈) 
4. [消息协议](#消息协议)
5. [数据库设计](#数据库设计)
6. [部署指南](#部署指南)
7. [开发规范](#开发规范)

## 核心功能
- 多角色人设管理
- 实时聊天交互
- 聊天历史记录
- 高并发网络通信
## 技术栈
### 服务端
- C++17
- 自定义TCP协议
- SQLite3数据库
### 客户端
- Qt框架
- JSON通信协议
### AI服务
- Python Flask
- DeepSeek API

### 主要消息

客户端→服务端 发送聊天消息 chat_response

服务端→客户端 返回AI响应 persona_list

服务端→客户端 返回人设列表

### 数据库设计

### 表结构 sqlite3

-- 人设表
CREATE TABLE personas (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    prompt TEXT NOT NULL
);

-- 会话表
CREATE TABLE conversations (
    id INTEGER PRIMARY KEY,
    persona_id TEXT NOT NULL,
    title TEXT NOT NULL
);
 ```