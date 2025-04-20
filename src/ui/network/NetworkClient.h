#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H
#include <string>

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();
    bool connectToServer(const std::string& ip, int port);
    void sendMessage(const std::string& message);
    std::string receiveMessage();
    void disconnectFromServer();

    int getSocketFd() const;

private:
    int socketFd;
}; // <-- 在这里添加分号

#endif // NETWORKCLIENT_H