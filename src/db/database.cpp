#include "database.h"


Database::Database(QString dbName, QObject *parent) : dbname(dbName), QObject(parent){
    db = QSqlDatabase::addDatabase("QSQLITE");
}

bool Database::initDatabase(){
    // 创建db目录(如果不存在)
    QString dbDirPath = QDir::currentPath() + "/../../../db"; // 从build目录向上两级到项目根目录
    QDir dbDir(dbDirPath);
    if(!dbDir.exists()){
        dbDir.mkpath(".");
    }
    
    QString dbPath = dbDirPath + "/" + dbname;
    qDebug() << "数据库存储位置:" << dbPath;
    
    db.setDatabaseName(dbPath);

    if(!db.open()){
        qWarning() << "数据库打开失败" << db.lastError().text();
        return false;
    }

    QSqlQuery query;
    // 启用外键支持
    query.exec("PRAGMA foreign_keys = ON");
    
    // 创建人设表
    query.exec("CREATE TABLE IF NOT EXISTS personas ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "description TEXT)");

    // 创建消息表，使用外键关联人设ID
    query.exec("CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "persona_id INTEGER NOT NULL, "
        "sender TEXT NOT NULL, "
        "message TEXT NOT NULL, "
        "message_length INTEGER NOT NULL, "
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "FOREIGN KEY(persona_id) REFERENCES personas(id) ON DELETE CASCADE)");
    
    // 为消息表创建索引，加速查询
    query.exec("CREATE INDEX IF NOT EXISTS idx_messages_persona_id ON messages(persona_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_messages_timestamp ON messages(timestamp)");

    // 检查是否需要升级现有的messages表结构
    QSqlQuery checkColumnQuery;
    checkColumnQuery.exec("PRAGMA table_info(messages)");
    bool hasMessageLength = false;
    while (checkColumnQuery.next()) {
        if (checkColumnQuery.value(1).toString() == "message_length") {
            hasMessageLength = true;
            break;
        }
    }

    // 如果不存在message_length列，则添加它
    if (!hasMessageLength) {
        QSqlQuery alterTableQuery;
        alterTableQuery.exec("ALTER TABLE messages ADD COLUMN message_length INTEGER DEFAULT 0");
        qDebug() << "已向messages表添加message_length列";
        
        // 更新现有记录的message_length
        QSqlQuery updateQuery;
        updateQuery.exec("UPDATE messages SET message_length = length(message)");
    }

    return true;
}

Database::~Database(){
    db.close();
}

int Database::addPersona(const QString &name, const QString &description){
    QSqlQuery query;
    query.prepare("INSERT INTO personas (name, description) VALUES (?, ?)");
    query.addBindValue(name);
    query.addBindValue(description);
    
    if(!query.exec()){
        qWarning() << "添加人设失败" << query.lastError().text();
        return -1; // 返回-1表示添加失败
    }
    
    // 获取新插入记录的ID
    int newId = query.lastInsertId().toInt();
    qDebug() << "添加人设成功，ID:" << newId;
    return newId;
}

bool Database::deletePersona(int personaId){
    // 开始事务
    db.transaction();
    
    // 先删除关联的聊天记录
    QSqlQuery deleteMessagesQuery;
    deleteMessagesQuery.prepare("DELETE FROM messages WHERE persona_id = ?");
    deleteMessagesQuery.addBindValue(personaId);
    if (!deleteMessagesQuery.exec()) {
        qWarning() << "删除聊天记录失败" << deleteMessagesQuery.lastError().text();
        db.rollback();
        return false;
    }
    
    // 再删除人设
    QSqlQuery deletePersonaQuery;
    deletePersonaQuery.prepare("DELETE FROM personas WHERE id = ?");
    deletePersonaQuery.addBindValue(personaId);
    if(!deletePersonaQuery.exec()){
        qWarning() << "删除人设失败" << deletePersonaQuery.lastError().text();
        db.rollback();
        return false;
    }
    
    // 提交事务
    db.commit();
    return true;
}



bool Database::addMessage(int personaId, const QString &sender, const QString &message){
    QSqlQuery query;
    query.prepare("INSERT INTO messages (persona_id, sender, message, message_length) VALUES (?,?,?,?)");
    query.addBindValue(personaId);
    query.addBindValue(sender);
    query.addBindValue(message);
    query.addBindValue(message.length()); // 添加消息长度
    
    if(!query.exec()){
        qWarning() << "添加消息失败" << query.lastError().text();
        return false;
    }
    return true;
}

QVector<QVector<QString>> Database::getChatHistory(int personaId){
    QVector<QVector<QString>> chatHistory;

    QSqlQuery query;
    query.prepare("SELECT sender, message, timestamp, message_length FROM messages WHERE persona_id = ? ORDER BY timestamp ASC");

    query.addBindValue(personaId);
    if(!query.exec()){
        qWarning() << "获取聊天记录失败" << query.lastError().text();
        return chatHistory;
    }
    while(query.next()){
        QVector<QString> record;
        record.push_back(query.value(0).toString()); // sender
        record.push_back(query.value(1).toString()); // message
        record.push_back(query.value(2).toString()); // timestamp
        record.push_back(query.value(3).toString()); // message_length
        chatHistory.push_back(record);    
    }
    return chatHistory;
}

// 获取最近的N条聊天记录
QVector<QVector<QString>> Database::getRecentChatHistory(int personaId, int limit) {
    QVector<QVector<QString>> chatHistory;

    QSqlQuery query;
    query.prepare("SELECT sender, message, timestamp, message_length FROM messages "
                 "WHERE persona_id = ? ORDER BY timestamp DESC LIMIT ?");
    query.addBindValue(personaId);
    query.addBindValue(limit);
    
    if(!query.exec()){
        qWarning() << "获取最近聊天记录失败" << query.lastError().text();
        return chatHistory;
    }
    
    // 结果是按时间倒序的，我们需要反转它以保持时间正序
    QVector<QVector<QString>> tempHistory;
    while(query.next()){
        QVector<QString> record;
        record.push_back(query.value(0).toString()); // sender
        record.push_back(query.value(1).toString()); // message
        record.push_back(query.value(2).toString()); // timestamp
        record.push_back(query.value(3).toString()); // message_length
        tempHistory.push_back(record);    
    }
    
    // 反转结果，使其按时间正序排列
    for (int i = tempHistory.size() - 1; i >= 0; i--) {
        chatHistory.push_back(tempHistory[i]);
    }
    
    return chatHistory;
}

// 获取最近N条消息的总长度
int Database::getTotalMessageLength(int personaId, int limit) {
    QSqlQuery query;
    query.prepare("SELECT SUM(message_length) FROM messages "
                 "WHERE persona_id = ? ORDER BY timestamp DESC LIMIT ?");
    query.addBindValue(personaId);
    query.addBindValue(limit);
    
    if(!query.exec() || !query.next()){
        qWarning() << "计算消息总长度失败" << query.lastError().text();
        return 0;
    }
    
    return query.value(0).toInt();
}

QVector<QPair<int, QString>> Database::getAllPersonas(){
    QVector<QPair<int, QString>> personas;
    QSqlQuery query;
    query.exec("SELECT id, name FROM personas");
    while(query.next()){
        int id = query.value(0).toInt();
        QString name = query.value(1).toString();
        personas.push_back(qMakePair(id, name));
    }
    return personas;
}

QPair<QString, QString> Database::getPersonaInfo(int personaId){
    QPair<QString, QString> info;
    QSqlQuery query;
    query.prepare("SELECT name, description FROM personas WHERE id = ?");
    query.addBindValue(personaId);
    if(query.exec() && query.next()){
        info.first = query.value(0).toString();
        info.second = query.value(1).toString();
    }
    return info;
}

