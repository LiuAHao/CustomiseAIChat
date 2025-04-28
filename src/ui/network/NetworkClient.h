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
    bool checkForResponse();
    void disconnectFromServer();
    bool reconnectToServer();

    int getSocketFd() const;

signals:
    void messageReceived(const std::string& message);
    void connectionStateChanged(bool connected);
    
private:
    void onSocketReadReady();
    
    int socketFd;
    bool messageWaiting;
    std::string serverIp;
    int serverPort;
    bool autoReconnect;
};

#endif // NETWORKCLIENT_H