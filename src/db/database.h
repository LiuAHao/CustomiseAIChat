#pragma once
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>

class Database : public QObject{
    Q_OBJECT
public:
    Database(QString dbName, QObject *parent = nullptr);
    ~Database();
    bool initDatabase();
    int addPersona(const QString &name, const QString &description);
    bool deletePersona(int personaId);
    bool addMessage(int personaId, const QString &sender, const QString &message);
    
    // 获取聊天记录的方法
    QVector<QVector<QString>> getChatHistory(int personaId);
    QVector<QVector<QString>> getRecentChatHistory(int personaId, int limit);
    int getTotalMessageLength(int personaId, int limit);
    
    QVector<QPair<int, QString>> getAllPersonas();
    QPair<QString, QString> getPersonaInfo(int personaId);
signals:
    void personaAdded(int personaId, const QString &name);
    void personaDeleted(int personaId);
    void messageAdded(int personaId, const QString &sender, const QString &message);

private:
    QSqlDatabase db;
    QString dbname;
};