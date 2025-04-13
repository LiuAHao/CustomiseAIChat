#include "database.h"


Database::Database(QString dbName, QObject *parent) : QObject(parent){
    db = QSqlDatabase::addDatabase("QSQLITE");
}

bool Database::initDatabase(){
    // 创建db目录(如果不存在)
    QString dbDirPath = QDir::currentPath() + "/../../../db"; // 从build目录向上两级到项目根目录
    QDir dbDir(dbDirPath);
    if(!dbDir.exists()){
        dbDir.mkpath(".");
    }
    
    QString dbPath = dbDirPath + "/aichat.db";
    qDebug() << "数据库存储位置:" << dbPath;
    
    db.setDatabaseName(dbPath);

    if(!db.open()){
        qWarning() << "数据库打开失败" << db.lastError().text();
        return false;
    }

    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS personas ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "name TEXT NOT NULL, "
        "description TEXT)");

    query.exec("CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "persona_id INTEGER NOT NULL, "
        "sender TEXT NOT NULL, "
        "message TEXT NOT NULL, "
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "FOREIGN KEY(persona_id) REFERENCES personas(id))");

    return true;
}

Database::~Database(){
    db.close();
}

bool Database::addPersona(const QString &name, const QString &description){
    QSqlQuery query;
    query.prepare("INSERT INTO personas (name, description) VALUES (?, ?)");
    query.addBindValue(name);
    query.addBindValue(description);
    if(!query.exec()){
        qWarning() << "添加人设失败" << query.lastError().text();
        return false;
    }
    return true;
}

bool Database::deletePersona(int personaId){
    QSqlQuery query;
    query.prepare("DELETE FROM personas WHERE id =?");
    query.addBindValue(personaId);
    if(!query.exec()){
        qWarning() << "删除人设失败" << query.lastError().text();
        return false;
    }
    return true;
}



bool Database::addMessage(int personaId, const QString &sender, const QString &message){
    QSqlQuery query;
    query.prepare("INSERT INTO messages (persona_id, sender, message) VALUES (?,?,?)");
    query.addBindValue(personaId);
    query.addBindValue(sender);
    query.addBindValue(message);
    if(!query.exec()){
        qWarning() << "添加消息失败" << query.lastError().text();
        return false;
    }
    return true;
}

QVector<QVector<QString>> Database::getChatHistory(const QString &personaId){
    QVector<QVector<QString>> chatHistory;

    QSqlQuery query;
    query.prepare("SELECT sender, message ,timestamp FROM messages WHERE persona_id = ? ORDER BY timestamp ASC");

    query.addBindValue(personaId);
    if(!query.exec()){
        qWarning() << "获取聊天记录失败" << query.lastError().text();
        return chatHistory;
    }
    while(query.next()){
        QVector<QString> record;
        record.push_back(query.value(0).toString());
        record.push_back(query.value(1).toString());
        record.push_back(query.value(2).toString());
        chatHistory.push_back(record);    
    }
    return chatHistory;
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
