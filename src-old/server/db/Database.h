#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <sqlite3.h>
#include "../model/User.h"
#include "../model/Persona.h"
#include "../model/Conversation.h"
#include "../model/Message.h"

class Database {
public:
    static Database& instance();

    bool init(const std::string& dbPath);
    void close();

    // ===== 用户操作 =====
    int createUser(const std::string& username, const std::string& passwordHash,
                   const std::string& salt, const std::string& nickname);
    bool getUserByUsername(const std::string& username, User& user);
    bool getUserById(int userId, User& user);
    bool updatePassword(int userId, const std::string& newHash, const std::string& newSalt);
    bool updateNickname(int userId, const std::string& nickname);
    bool deleteUser(int userId);

    // ===== 人设操作 =====
    int createPersona(int userId, const std::string& name, const std::string& description,
                      const std::string& systemPrompt, const std::string& avatar = "");
    bool deletePersona(int personaId, int userId);
    std::vector<Persona> getPersonasByUser(int userId);
    bool getPersona(int personaId, int userId, Persona& persona);
    bool updatePersona(int personaId, int userId, const std::string& name,
                       const std::string& description, const std::string& systemPrompt,
                       const std::string& avatar = "");

    // ===== 会话操作 =====
    int createConversation(int userId, int personaId, const std::string& title);
    bool deleteConversation(int conversationId, int userId);
    bool getConversation(int conversationId, int userId, Conversation& conv);
    std::vector<Conversation> getConversationsByUser(int userId, int limit = 50);
    std::vector<Conversation> getConversationsByPersona(int userId, int personaId);
    bool updateConversationTitle(int conversationId, int userId, const std::string& title);
    bool updateConversationTimestamp(int conversationId);
    bool incrementMessageCount(int conversationId);

    // ===== 消息操作 =====
    bool addMessage(int conversationId, const std::string& role, const std::string& content);
    std::vector<ChatMessage> getMessages(int conversationId, int limit = 50);
    std::vector<ChatMessage> getRecentMessages(int conversationId, int limit);
    bool clearMessages(int conversationId, int userId);
    int getMessageCount(int conversationId);

    // ===== 权限校验辅助 =====
    bool isConversationOwner(int conversationId, int userId);
    bool isPersonaOwner(int personaId, int userId);

private:
    Database() = default;
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool createTables();
    bool execSQL(const char* sql);
    bool execSQLSilent(const char* sql);  // 静默执行，失败不打印日志

    sqlite3* db_ = nullptr;
    std::mutex mutex_;
};
