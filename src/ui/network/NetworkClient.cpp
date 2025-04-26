#include "NetworkClient.h"

#include<sys/socket.h>
#include<netinet/in.h>

#include<arpa/inet.h>
#include<unistd.h>
#include<vector>
#include<errno.h>
#include<string.h>
#include<cstdio>
#include<QDebug>       // <-- 将 QDbug 改为 QDebug

NetworkClient::NetworkClient(QObject* parent) : QObject(parent), socketFd(-1) {}

NetworkClient::~NetworkClient() {
    if (socketFd != -1) {
        ::close(socketFd);
        socketFd = -1;
    }
}

bool NetworkClient::connectToServer(const std::string& ip, int port) {
    socketFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if(socketFd == -1) {
        qDebug() << "Failed to create socket";
        return false;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if(::inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
        qDebug() << "Invalid IP address";
        ::close(socketFd);
        socketFd = -1;
        return false;
    }

    if(::connect(socketFd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        qDebug() << "Failed to connect to server";
        ::close(socketFd);
        socketFd = -1;
        return false;
    }
    return true;
}

void NetworkClient::sendMessage(const std::string& message){
    if(socketFd == -1) {
        qDebug() << "[NetworkClient] Socket not initialized"; // 添加日志标识
        return;
    }

    char tmpbuf[1024];
    memset(tmpbuf,0,sizeof(tmpbuf));
    int len = strlen(message.c_str());

    memcpy(tmpbuf,&len,4);          //拼接头部信息，第二个为填入的内容，第三个为占据的字节数
    memcpy(tmpbuf + 4,message.data(),len);    //拼接内容
    send(socketFd,tmpbuf,len + 4,0);

    
}

std::string NetworkClient::receiveMessage(){
    if(socketFd == -1) {
        qDebug() << "[NetworkClient] Socket not initialized";
        return "";
    }

    std::string inputbuffer;
    
    char buffer[1024];
    while(true) //一次读取buffer数据，直到读取完成
    {    
        bzero(&buffer,sizeof(buffer));
        ssize_t nread = ::read(socketFd,buffer,sizeof(buffer));    //nread代表成功读取到的字节数
        if(nread > 0)
        {     
            inputbuffer.append(buffer,nread);
        }
        else if(nread == 0){
            break;
        }
        else if(nread == -1){   
            break;
        }
    }
    

    if(inputbuffer.size() <= 4) {
        qDebug() << "[NetworkClient] Received message too short:" << inputbuffer.size() << "bytes";
        return "";
    }
    
    // 跳过前8个字节的头部信息，只返回实际的JSON内容
    std::string jsonContent = inputbuffer.substr(4);
    qDebug() << "[NetworkClient] Extracted JSON content, length:" << jsonContent.size() << "bytes";
    
    return jsonContent;
}

void NetworkClient::disconnectFromServer(){
    if(socketFd == -1) {
        qDebug() << "Socket not initialized";
        return;
    }
    ::close(socketFd);
    socketFd = -1;
}

int NetworkClient::getSocketFd() const {
    return socketFd;
}

