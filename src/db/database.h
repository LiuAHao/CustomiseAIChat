#pramma once
#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>

class Database : public QObject{
    Q_OBJECT
public:
    Database(QObject *parent = nullptr);
    ~Database();
    bool initDatabase();
    bool addPersona(const QString &name, const QString &description);
    bool addMessage(int personaId, const QString &sender, const QString &message);
    QVector<QVector<QString>> getChatHistory(const QString &personaId);
    QVector<QPair<int, QString>> getAllPersonas();

private:
    QSqlDatabase db;
}