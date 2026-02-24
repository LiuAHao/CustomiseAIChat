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

bool Database::createTables() {
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

    const char* personaTable =
        "CREATE TABLE IF NOT EXISTS personas ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  user_id INTEGER NOT NULL,"
        "  name TEXT NOT NULL,"
        "  description TEXT DEFAULT '',"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");";

    const char* messageTable =
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  persona_id INTEGER NOT NULL,"
        "  user_id INTEGER NOT NULL,"
        "  sender TEXT NOT NULL,"
        "  content TEXT NOT NULL,"
        "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY(persona_id) REFERENCES personas(id) ON DELETE CASCADE,"
        "  FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");";

    const char* indexPersonaUser =
        "CREATE INDEX IF NOT EXISTS idx_personas_user ON personas(user_id);";
    const char* indexMessagePersona =
        "CREATE INDEX IF NOT EXISTS idx_messages_persona ON messages(persona_id);";
    const char* indexMessageUser =
        "CREATE INDEX IF NOT EXISTS idx_messages_user ON messages(user_id);";
    const char* indexMessageTime =
        "CREATE INDEX IF NOT EXISTS idx_messages_timestamp ON messages(timestamp);";

    return execSQL(userTable) && execSQL(personaTable) && execSQL(messageTable)
        && execSQL(indexPersonaUser) && execSQL(indexMessagePersona)
        && execSQL(indexMessageUser) && execSQL(indexMessageTime);
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

    // 开启事务，级联删除用户相关数据
    execSQL("BEGIN TRANSACTION");

    const char* delMsg = "DELETE FROM messages WHERE user_id = ?";
    const char* delPersona = "DELETE FROM personas WHERE user_id = ?";
    const char* delUser = "DELETE FROM users WHERE id = ?";

    sqlite3_stmt* stmt = nullptr;
    bool ok = true;

    // 删除消息
    if (sqlite3_prepare_v2(db_, delMsg, -1, &stmt, nullptr) == SQLITE_OK) {
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

int Database::createPersona(int userId, const std::string& name, const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "INSERT INTO personas (user_id, name, description) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, description.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) return -1;
    return static_cast<int>(sqlite3_last_insert_rowid(db_));
}

bool Database::deletePersona(int personaId, int userId) {
    std::lock_guard<std::mutex> lock(mutex_);

    execSQL("BEGIN TRANSACTION");

    // 先删除关联消息
    const char* delMsg = "DELETE FROM messages WHERE persona_id = ? AND user_id = ?";
    sqlite3_stmt* stmt = nullptr;
    bool ok = true;

    if (sqlite3_prepare_v2(db_, delMsg, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, personaId);
        sqlite3_bind_int(stmt, 2, userId);
        ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
    }

    // 再删除人设
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

    const char* sql = "SELECT id, user_id, name, description, created_at FROM personas WHERE user_id = ? ORDER BY id";
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
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        p.createdAt = ca ? ca : "";
        personas.push_back(p);
    }

    sqlite3_finalize(stmt);
    return personas;
}

bool Database::getPersona(int personaId, int userId, Persona& persona) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "SELECT id, user_id, name, description, created_at FROM personas WHERE id = ? AND user_id = ?";
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
        const char* ca = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        persona.createdAt = ca ? ca : "";
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

bool Database::updatePersona(int personaId, int userId,
                             const std::string& name, const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "UPDATE personas SET name = ?, description = ? WHERE id = ? AND user_id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, personaId);
    sqlite3_bind_int(stmt, 4, userId);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ===== 消息操作 =====

bool Database::addMessage(int personaId, int userId, const std::string& sender, const std::string& content) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "INSERT INTO messages (persona_id, user_id, sender, content) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, personaId);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_bind_text(stmt, 3, sender.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, content.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

std::vector<ChatMessage> Database::getMessages(int personaId, int userId, int limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ChatMessage> messages;

    const char* sql = "SELECT id, persona_id, user_id, sender, content, timestamp "
                      "FROM messages WHERE persona_id = ? AND user_id = ? "
                      "ORDER BY timestamp ASC LIMIT ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return messages;

    sqlite3_bind_int(stmt, 1, personaId);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_bind_int(stmt, 3, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ChatMessage msg;
        msg.id = sqlite3_column_int(stmt, 0);
        msg.personaId = sqlite3_column_int(stmt, 1);
        msg.userId = sqlite3_column_int(stmt, 2);
        msg.sender = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        msg.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char* ts = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        msg.timestamp = ts ? ts : "";
        messages.push_back(msg);
    }

    sqlite3_finalize(stmt);
    return messages;
}

std::vector<ChatMessage> Database::getRecentMessages(int personaId, int userId, int limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ChatMessage> messages;

    // 先倒序取最近N条，再正序返回
    const char* sql = "SELECT id, persona_id, user_id, sender, content, timestamp FROM ("
                      "  SELECT id, persona_id, user_id, sender, content, timestamp "
                      "  FROM messages WHERE persona_id = ? AND user_id = ? "
                      "  ORDER BY timestamp DESC LIMIT ?"
                      ") ORDER BY timestamp ASC";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return messages;

    sqlite3_bind_int(stmt, 1, personaId);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_bind_int(stmt, 3, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ChatMessage msg;
        msg.id = sqlite3_column_int(stmt, 0);
        msg.personaId = sqlite3_column_int(stmt, 1);
        msg.userId = sqlite3_column_int(stmt, 2);
        msg.sender = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        msg.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        const char* ts = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        msg.timestamp = ts ? ts : "";
        messages.push_back(msg);
    }

    sqlite3_finalize(stmt);
    return messages;
}

bool Database::clearMessages(int personaId, int userId) {
    std::lock_guard<std::mutex> lock(mutex_);

    const char* sql = "DELETE FROM messages WHERE persona_id = ? AND user_id = ?";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, personaId);
    sqlite3_bind_int(stmt, 2, userId);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}
