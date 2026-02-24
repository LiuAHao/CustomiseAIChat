#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <sqlite3.h>
#include "../model/User.h"
#include "../model/Persona.h"
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
    int createPersona(int userId, const std::string& name, const std::string& description);
    bool deletePersona(int personaId, int userId);
    std::vector<Persona> getPersonasByUser(int userId);
    bool getPersona(int personaId, int userId, Persona& persona);
    bool updatePersona(int personaId, int userId, const std::string& name, const std::string& description);

    // ===== 消息操作 =====
    bool addMessage(int personaId, int userId, const std::string& sender, const std::string& content);
    std::vector<ChatMessage> getMessages(int personaId, int userId, int limit = 50);
    std::vector<ChatMessage> getRecentMessages(int personaId, int userId, int limit);
    bool clearMessages(int personaId, int userId);

private:
    Database() = default;
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool createTables();
    bool execSQL(const char* sql);

    sqlite3* db_ = nullptr;
    std::mutex mutex_;
};
