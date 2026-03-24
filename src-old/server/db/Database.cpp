#include "Database.h"
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>

Database& Database::instance() {
    static Database db;
    return db;
}

Database::~Database() {
    close();
}

static void createDirectoryRecursive(const std::string& path) {
    size_t pos = 0;
    do {
        pos = path.find('/', pos + 1);
        std::string subdir = path.substr(0, pos);
        if (!subdir.empty()) {
            mkdir(subdir.c_str(), 0755);
        }
    } while (pos != std::string::npos);
}

bool Database::init(const std::string& dbPath) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 创建数据库所在目录
    size_t lastSlash = dbPath.find_last_of('/');
    if (lastSlash != std::string::npos) {
        createDirectoryRecursive(dbPath.substr(0, lastSlash));
    }

    int rc = sqlite3_open(dbPath.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database] 打开数据库失败: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    // 启用WAL模式提升并发性能
    execSQL("PRAGMA journal_mode=WAL");
    execSQL("PRAGMA foreign_keys=ON");

    if (!createTables()) {
        std::cerr << "[Database] 创建数据表失败" << std::endl;
        return false;
    }

    std::cout << "[Database] 数据库初始化成功: " << dbPath << std::endl;
    return true;
}

void Database::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::execSQL(const char* sql) {
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database] SQL执行失败: " << errmsg << std::endl;
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

bool Database::execSQLSilent(const char* sql) {
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

bool Database::createTables() {
    // 用户表
    const char* userTable =
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username TEXT NOT NULL UNIQUE,"
        "  password_hash TEXT NOT NULL,"
        "  salt TEXT NOT NULL,"
        "  nickname TEXT NOT NULL,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");";

    // 人设表：增加 system_prompt、avatar、updated_at
    const char* personaTable =
        "CREATE TABLE IF NOT EXISTS personas ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  user_id INTEGER NOT NULL,"
        "  name TEXT NOT NULL,"
        "  description TEXT DEFAULT '',"
        "  system_prompt TEXT DEFAULT '',"
        "  avatar TEXT DEFAULT '',"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");";

    // 会话表：连接用户和人设，承载消息
    const char* conversationTable =
        "CREATE TABLE IF NOT EXISTS conversations ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  user_id INTEGER NOT NULL,"
        "  persona_id INTEGER NOT NULL,"
        "  title TEXT DEFAULT '新会话',"
        "  message_count INTEGER DEFAULT 0,"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE,"
        "  FOREIGN KEY(persona_id) REFERENCES personas(id) ON DELETE CASCADE"
        ");";

    // 消息表：仅关联到会话
    const char* messageTable =
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  conversation_id INTEGER NOT NULL,"
        "  role TEXT NOT NULL,"
        "  content TEXT NOT NULL,"
        "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY(conversation_id) REFERENCES conversations(id) ON DELETE CASCADE"
        ");";

    // 索引
    const char* idxPersonaUser =
        "CREATE INDEX IF NOT EXISTS idx_personas_user ON personas(user_id);";
    const char* idxConvUser =
        "CREATE INDEX IF NOT EXISTS idx_conversations_user ON conversations(user_id);";
    const char* idxConvPersona =
        "CREATE INDEX IF NOT EXISTS idx_conversations_persona ON conversations(persona_id);";
    const char* idxConvUpdated =
        "CREATE INDEX IF NOT EXISTS idx_conversations_updated ON conversations(updated_at);";
    const char* idxMsgConv =
        "CREATE INDEX IF NOT EXISTS idx_messages_conversation ON messages(conversation_id);";
    const char* idxMsgTime =
        "CREATE INDEX IF NOT EXISTS idx_messages_timestamp ON messages(timestamp);";

    // 旧表兼容：为 personas 表添加新列（如果已存在则静默忽略）
    execSQLSilent("ALTER TABLE personas ADD COLUMN system_prompt TEXT DEFAULT ''");
    execSQLSilent("ALTER TABLE personas ADD COLUMN avatar TEXT DEFAULT ''");
    execSQLSilent("ALTER TABLE personas ADD COLUMN updated_at DATETIME DEFAULT CURRENT_TIMESTAMP");

    return execSQL(userTable) && execSQL(personaTable)
        && execSQL(conversationTable) && execSQL(messageTable)
        && execSQL(idxPersonaUser) && execSQL(idxConvUser)
        && execSQL(idxConvPersona) && execSQL(idxConvUpdated)
        && execSQL(idxMsgConv) && execSQL(idxMsgTime);
}

// ===== 用户操作 =====

int Database::createUser(const std::string& username, const std::string& passwordHash,
                         const std::string& salt, const std::string& nickname) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "INSERT INTO users (username, password_hash, salt, nickname) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Database] createUser prepare失败: " << sqlite3_errmsg(db_) << std::endl;
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, salt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, nickname.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "[Database] createUser失败: " << sqlite3_errmsg(db_) << std::endl;
        return -1;
    }

    return static_cast<int>(sqlite3_last_insert_rowid(db_));
}

bool Database::getUserByUsername(const std::string& username, User& user) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT id, username, password_hash, salt, nickname, created_at, updated_at "
                      "FROM users WHERE username = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.passwordHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user.nickname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* ua = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        user.createdAt = ca ? ca : "";
        user.updatedAt = ua ? ua : "";
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

bool Database::getUserById(int userId, User& user) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT id, username, password_hash, salt, nickname, created_at, updated_at "
                      "FROM users WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, userId);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        user.username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        user.passwordHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        user.salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        user.nickname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* ua = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        user.createdAt = ca ? ca : "";
        user.updatedAt = ua ? ua : "";
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

bool Database::updatePassword(int userId, const std::string& newHash, const std::string& newSalt) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "UPDATE users SET password_hash = ?, salt = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, newHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, newSalt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, userId);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::updateNickname(int userId, const std::string& nickname) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "UPDATE users SET nickname = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, nickname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, userId);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::deleteUser(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 开启事务，级联删除用户相关所有数据
    execSQL("BEGIN TRANSACTION");

    const char* delMsg = "DELETE FROM messages WHERE conversation_id IN "
                         "(SELECT id FROM conversations WHERE user_id = ?)";
    const char* delConv = "DELETE FROM conversations WHERE user_id = ?";
    const char* delPersona = "DELETE FROM personas WHERE user_id = ?";
    const char* delUser = "DELETE FROM users WHERE id = ?";

    sqlite3_stmt* stmt = nullptr;
    bool ok = true;

    // 删除消息（通过会话关联）
    if (sqlite3_prepare_v2(db_, delMsg, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, userId);
        ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }

    // 删除会话
    if (ok && sqlite3_prepare_v2(db_, delConv, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, userId);
        ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }

    // 删除人设
    if (ok && sqlite3_prepare_v2(db_, delPersona, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, userId);
        ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }

    // 删除用户
    if (ok && sqlite3_prepare_v2(db_, delUser, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, userId);
        ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }

    if (ok) {
        execSQL("COMMIT");
    } else {
        execSQL("ROLLBACK");
    }
    return ok;
}

// ===== 人设操作 =====

int Database::createPersona(int userId, const std::string& name, const std::string& description,
                            const std::string& systemPrompt, const std::string& avatar) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "INSERT INTO personas (user_id, name, description, system_prompt, avatar) "
                      "VALUES (?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, systemPrompt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, avatar.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return -1;
    return static_cast<int>(sqlite3_last_insert_rowid(db_));
}

bool Database::deletePersona(int personaId, int userId) {
    std::lock_guard<std::mutex> lock(mutex_);

    execSQL("BEGIN TRANSACTION");

    bool ok = true;
    sqlite3_stmt* stmt = nullptr;

    // 1. 删除该人设下所有会话的消息
    const char* delMsg = "DELETE FROM messages WHERE conversation_id IN "
                         "(SELECT id FROM conversations WHERE persona_id = ? AND user_id = ?)";
    if (sqlite3_prepare_v2(db_, delMsg, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, personaId);
        sqlite3_bind_int(stmt, 2, userId);
        ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }

    // 2. 删除该人设下所有会话
    if (ok) {
        const char* delConv = "DELETE FROM conversations WHERE persona_id = ? AND user_id = ?";
        if (sqlite3_prepare_v2(db_, delConv, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, personaId);
            sqlite3_bind_int(stmt, 2, userId);
            ok = (sqlite3_step(stmt) == SQLITE_DONE);
            sqlite3_finalize(stmt);
        }
    }

    // 3. 删除人设
    if (ok) {
        const char* delPersona = "DELETE FROM personas WHERE id = ? AND user_id = ?";
        if (sqlite3_prepare_v2(db_, delPersona, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, personaId);
            sqlite3_bind_int(stmt, 2, userId);
            ok = (sqlite3_step(stmt) == SQLITE_DONE);
            sqlite3_finalize(stmt);
        }
    }

    if (ok) execSQL("COMMIT");
    else execSQL("ROLLBACK");
    return ok;
}

std::vector<Persona> Database::getPersonasByUser(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Persona> personas;

    const char* sql = "SELECT id, user_id, name, description, system_prompt, avatar, "
                      "created_at, updated_at FROM personas WHERE user_id = ? ORDER BY updated_at DESC";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return personas;

    sqlite3_bind_int(stmt, 1, userId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Persona p;
        p.id = sqlite3_column_int(stmt, 0);
        p.userId = sqlite3_column_int(stmt, 1);
        p.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        p.description = desc ? desc : "";
        const char* sp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        p.systemPrompt = sp ? sp : "";
        const char* av = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        p.avatar = av ? av : "";
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        p.createdAt = ca ? ca : "";
        const char* ua = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        p.updatedAt = ua ? ua : "";
        personas.push_back(p);
    }

    sqlite3_finalize(stmt);
    return personas;
}

bool Database::getPersona(int personaId, int userId, Persona& persona) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT id, user_id, name, description, system_prompt, avatar, "
                      "created_at, updated_at FROM personas WHERE id = ? AND user_id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, personaId);
    sqlite3_bind_int(stmt, 2, userId);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        persona.id = sqlite3_column_int(stmt, 0);
        persona.userId = sqlite3_column_int(stmt, 1);
        persona.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        persona.description = desc ? desc : "";
        const char* sp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        persona.systemPrompt = sp ? sp : "";
        const char* av = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        persona.avatar = av ? av : "";
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        persona.createdAt = ca ? ca : "";
        const char* ua = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        persona.updatedAt = ua ? ua : "";
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

bool Database::updatePersona(int personaId, int userId, const std::string& name,
                             const std::string& description, const std::string& systemPrompt,
                             const std::string& avatar) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "UPDATE personas SET name = ?, description = ?, system_prompt = ?, "
                      "avatar = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ? AND user_id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, systemPrompt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, avatar.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, personaId);
    sqlite3_bind_int(stmt, 6, userId);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ===== 会话操作 =====

int Database::createConversation(int userId, int personaId, const std::string& title) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "INSERT INTO conversations (user_id, persona_id, title) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, personaId);
    sqlite3_bind_text(stmt, 3, title.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return -1;
    return static_cast<int>(sqlite3_last_insert_rowid(db_));
}

bool Database::deleteConversation(int conversationId, int userId) {
    std::lock_guard<std::mutex> lock(mutex_);

    execSQL("BEGIN TRANSACTION");

    bool ok = true;
    sqlite3_stmt* stmt = nullptr;

    // 先删除会话下的消息
    const char* delMsg = "DELETE FROM messages WHERE conversation_id = ?";
    if (sqlite3_prepare_v2(db_, delMsg, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, conversationId);
        ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }

    // 再删除会话本身（带权限校验）
    if (ok) {
        const char* delConv = "DELETE FROM conversations WHERE id = ? AND user_id = ?";
        if (sqlite3_prepare_v2(db_, delConv, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, conversationId);
            sqlite3_bind_int(stmt, 2, userId);
            ok = (sqlite3_step(stmt) == SQLITE_DONE);
            sqlite3_finalize(stmt);
        }
    }

    if (ok) execSQL("COMMIT");
    else execSQL("ROLLBACK");
    return ok;
}

bool Database::getConversation(int conversationId, int userId, Conversation& conv) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT id, user_id, persona_id, title, message_count, "
                      "created_at, updated_at FROM conversations WHERE id = ? AND user_id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, conversationId);
    sqlite3_bind_int(stmt, 2, userId);

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        conv.id = sqlite3_column_int(stmt, 0);
        conv.userId = sqlite3_column_int(stmt, 1);
        conv.personaId = sqlite3_column_int(stmt, 2);
        const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        conv.title = t ? t : "";
        conv.messageCount = sqlite3_column_int(stmt, 4);
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        conv.createdAt = ca ? ca : "";
        const char* ua = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        conv.updatedAt = ua ? ua : "";
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

std::vector<Conversation> Database::getConversationsByUser(int userId, int limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Conversation> convs;

    const char* sql = "SELECT c.id, c.user_id, c.persona_id, c.title, c.message_count, "
                      "c.created_at, c.updated_at "
                      "FROM conversations c "
                      "WHERE c.user_id = ? ORDER BY c.updated_at DESC LIMIT ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return convs;

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Conversation conv;
        conv.id = sqlite3_column_int(stmt, 0);
        conv.userId = sqlite3_column_int(stmt, 1);
        conv.personaId = sqlite3_column_int(stmt, 2);
        const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        conv.title = t ? t : "";
        conv.messageCount = sqlite3_column_int(stmt, 4);
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        conv.createdAt = ca ? ca : "";
        const char* ua = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        conv.updatedAt = ua ? ua : "";
        convs.push_back(conv);
    }

    sqlite3_finalize(stmt);
    return convs;
}

std::vector<Conversation> Database::getConversationsByPersona(int userId, int personaId) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Conversation> convs;

    const char* sql = "SELECT id, user_id, persona_id, title, message_count, "
                      "created_at, updated_at FROM conversations "
                      "WHERE user_id = ? AND persona_id = ? ORDER BY updated_at DESC";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return convs;

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, personaId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Conversation conv;
        conv.id = sqlite3_column_int(stmt, 0);
        conv.userId = sqlite3_column_int(stmt, 1);
        conv.personaId = sqlite3_column_int(stmt, 2);
        const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        conv.title = t ? t : "";
        conv.messageCount = sqlite3_column_int(stmt, 4);
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        conv.createdAt = ca ? ca : "";
        const char* ua = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        conv.updatedAt = ua ? ua : "";
        convs.push_back(conv);
    }

    sqlite3_finalize(stmt);
    return convs;
}

bool Database::updateConversationTitle(int conversationId, int userId, const std::string& title) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "UPDATE conversations SET title = ?, updated_at = CURRENT_TIMESTAMP "
                      "WHERE id = ? AND user_id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, conversationId);
    sqlite3_bind_int(stmt, 3, userId);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::updateConversationTimestamp(int conversationId) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "UPDATE conversations SET updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, conversationId);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::incrementMessageCount(int conversationId) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "UPDATE conversations SET message_count = message_count + 1, "
                      "updated_at = CURRENT_TIMESTAMP WHERE id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, conversationId);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ===== 消息操作 =====

bool Database::addMessage(int conversationId, const std::string& role, const std::string& content) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "INSERT INTO messages (conversation_id, role, content) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, conversationId);
    sqlite3_bind_text(stmt, 2, role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc == SQLITE_DONE) {
        // 更新会话的消息计数和时间戳（在同一锁内直接执行）
        const char* updateSql = "UPDATE conversations SET message_count = message_count + 1, "
                                "updated_at = CURRENT_TIMESTAMP WHERE id = ?";
        sqlite3_stmt* updateStmt = nullptr;
        if (sqlite3_prepare_v2(db_, updateSql, -1, &updateStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(updateStmt, 1, conversationId);
            sqlite3_step(updateStmt);
            sqlite3_finalize(updateStmt);
        }
        return true;
    }
    return false;
}

std::vector<ChatMessage> Database::getMessages(int conversationId, int limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ChatMessage> messages;

    const char* sql = "SELECT id, conversation_id, role, content, timestamp "
                      "FROM messages WHERE conversation_id = ? "
                      "ORDER BY timestamp ASC LIMIT ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return messages;

    sqlite3_bind_int(stmt, 1, conversationId);
    sqlite3_bind_int(stmt, 2, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ChatMessage msg;
        msg.id = sqlite3_column_int(stmt, 0);
        msg.conversationId = sqlite3_column_int(stmt, 1);
        msg.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        msg.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* ts = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        msg.timestamp = ts ? ts : "";
        messages.push_back(msg);
    }

    sqlite3_finalize(stmt);
    return messages;
}

std::vector<ChatMessage> Database::getRecentMessages(int conversationId, int limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ChatMessage> messages;

    // 先倒序取最近N条，再正序返回
    const char* sql = "SELECT id, conversation_id, role, content, timestamp FROM ("
                      "  SELECT id, conversation_id, role, content, timestamp "
                      "  FROM messages WHERE conversation_id = ? "
                      "  ORDER BY timestamp DESC LIMIT ?"
                      ") ORDER BY timestamp ASC";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return messages;

    sqlite3_bind_int(stmt, 1, conversationId);
    sqlite3_bind_int(stmt, 2, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ChatMessage msg;
        msg.id = sqlite3_column_int(stmt, 0);
        msg.conversationId = sqlite3_column_int(stmt, 1);
        msg.role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        msg.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* ts = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        msg.timestamp = ts ? ts : "";
        messages.push_back(msg);
    }

    sqlite3_finalize(stmt);
    return messages;
}

bool Database::clearMessages(int conversationId, int userId) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 先校验会话归属
    const char* checkSql = "SELECT id FROM conversations WHERE id = ? AND user_id = ?";
    sqlite3_stmt* checkStmt = nullptr;
    if (sqlite3_prepare_v2(db_, checkSql, -1, &checkStmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(checkStmt, 1, conversationId);
    sqlite3_bind_int(checkStmt, 2, userId);
    bool owned = (sqlite3_step(checkStmt) == SQLITE_ROW);
    sqlite3_finalize(checkStmt);
    if (!owned) return false;

    execSQL("BEGIN TRANSACTION");

    // 清空消息
    const char* sql = "DELETE FROM messages WHERE conversation_id = ?";
    sqlite3_stmt* stmt = nullptr;
    bool ok = true;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, conversationId);
        ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }

    // 重置消息计数
    if (ok) {
        const char* resetSql = "UPDATE conversations SET message_count = 0, "
                               "updated_at = CURRENT_TIMESTAMP WHERE id = ?";
        sqlite3_stmt* resetStmt = nullptr;
        if (sqlite3_prepare_v2(db_, resetSql, -1, &resetStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(resetStmt, 1, conversationId);
            ok = (sqlite3_step(resetStmt) == SQLITE_DONE);
            sqlite3_finalize(resetStmt);
        }
    }

    if (ok) execSQL("COMMIT");
    else execSQL("ROLLBACK");
    return ok;
}

int Database::getMessageCount(int conversationId) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT COUNT(*) FROM messages WHERE conversation_id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return 0;

    sqlite3_bind_int(stmt, 1, conversationId);

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count;
}

// ===== 权限校验辅助 =====

bool Database::isConversationOwner(int conversationId, int userId) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT id FROM conversations WHERE id = ? AND user_id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, conversationId);
    sqlite3_bind_int(stmt, 2, userId);

    bool owned = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return owned;
}

bool Database::isPersonaOwner(int personaId, int userId) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT id FROM personas WHERE id = ? AND user_id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, personaId);
    sqlite3_bind_int(stmt, 2, userId);

    bool owned = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return owned;
}
