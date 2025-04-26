#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H
#include <string>
#include <QObject>

class NetworkClient :public QObject{
    Q_OBJECT
public:
    NetworkClient(QObject* parent = nullptr);
    ~NetworkClient();
    bool connectToServer(const std::string& ip, int port);
    void sendMessage(const std::string& message);
    std::string receiveMessage();
    void disconnectFromServer();

    int getSocketFd() const;

signals:
    void messageReceived(const std::string& message);
    
private:
    int socketFd;
};

#endif // NETWORKCLIENT_H