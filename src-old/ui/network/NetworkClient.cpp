#include "NetworkClient.h"

#include<sys/socket.h>
#include<netinet/in.h>

#include<arpa/inet.h>
#include<unistd.h>
#include<vector>
#include<errno.h>
#include<string.h>
#include<cstdio>
#include<QDebug> 
#include<fcntl.h>
#include<QThread>

NetworkClient::NetworkClient(QObject* parent) 
    : QObject(parent)
    , socketFd(-1)
    , messageWaiting(false)
    , serverIp("")
    , serverPort(0)
    , autoReconnect(true){}

NetworkClient::~NetworkClient() {
    if (socketFd != -1) {
        disconnectFromServer();
    }
}

bool NetworkClient::connectToServer(const std::string& ip, int port) {
    // 保存服务器 IP 和端口，以便后续重连
    serverIp = ip;
    serverPort = port;

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
    
    // 设置套接字为非阻塞模式
    int flags = fcntl(socketFd, F_GETFL, 0);
    if (flags == -1) {
        qDebug() << "Failed to get socket flags";
        ::close(socketFd);
        socketFd = -1;
        return false;
    }

    if (fcntl(socketFd, F_SETFL, flags | O_NONBLOCK) == -1) {
        qDebug() << "Failed to set socket to non-blocking mode";
        ::close(socketFd);
        socketFd = -1;
        return false;
    }

    return true;
}

void NetworkClient::sendMessage(const std::string& message){
    if(socketFd == -1) {
        return;
    }

    char tmpbuf[1024];
    memset(tmpbuf,0,sizeof(tmpbuf));
    int len = message.size(); 

    
    // 将长度信息写入头部4个字节
    memcpy(tmpbuf, &len, 4);
    // 将消息内容复制到缓冲区
    memcpy(tmpbuf + 4, message.data(), len);
    
    // 发送完整消息（头部+内容）
    ssize_t sent = send(socketFd, tmpbuf, len + 4, 0);
    if(sent == -1) {
        qDebug() << "[NetworkClient] Failed to send message:" << strerror(errno);
    } else if(sent < len + 4) {
        qDebug() << "[NetworkClient] Partial message sent:" << sent << "bytes of" << (len + 4);
    } else {
        messageWaiting = true;
    }
}

// 检查是否有响应并处理
bool NetworkClient::checkForResponse() {
    if (!messageWaiting || socketFd == -1) {
        return false; // 没有正在等待的消息或套接字已关闭
    }
    
    // 检查套接字是否有数据可读
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(socketFd, &readSet);
    
    // 设置超时时间为0，立即返回
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    
    int result = select(socketFd + 1, &readSet, nullptr, nullptr, &timeout);
    
    if (result > 0 && FD_ISSET(socketFd, &readSet)) {
        // 有数据可读
        onSocketReadReady();
        return true;
    }
    
    return false; // 没有数据可读
}

void NetworkClient::onSocketReadReady() {
    
    std::string message = receiveMessage();
    if (!message.empty()) {
        messageWaiting = false; // 重置标志，我们已经收到响应
        emit messageReceived(message);
    } else {
        qDebug() << "[NetworkClient] No message received or empty message";
    }
}

std::string NetworkClient::receiveMessage(){
    if(socketFd == -1) {
        qDebug() << "[NetworkClient] Socket not initialized";
        return "";
    }

    std::string inputbuffer;
    bool connectionClosed = false;
    
    // 设置接收超时时间，防止一直阻塞（虽然我们使用非阻塞模式，但增加额外保护）
    int maxAttempts = 10;  // 最大尝试次数
    int attempts = 0;
    
    char buffer[1024];
    while(attempts < maxAttempts) //一次读取buffer数据，限制尝试次数
    {    
        attempts++;
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = ::read(socketFd, buffer, sizeof(buffer));
        
        if(nread > 0) {     
            inputbuffer.append(buffer, nread);
            
            // 如果已经读取了足够的数据，可以退出循环
            if(inputbuffer.size() > 4) {
                // 尝试解析前4个字节中的长度
                int expectedLength = 0;
                memcpy(&expectedLength, inputbuffer.data(), 4);
                
                // 如果已经读取了足够长度的数据，退出循环
                if(inputbuffer.size() >= 4 + expectedLength) {
                    break;
                }
            }
        }
        else if(nread == 0) {
            qDebug() << "[NetworkClient] End of file reached";
            connectionClosed = true;
            break;
        }
        else if(nread == -1) {   
            // EWOULDBLOCK 或 EAGAIN 表示暂时没有数据，非阻塞模式下可能需要稍后再尝试
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                qDebug() << "[NetworkClient] No more data available right now";
                // 如果已经有一些数据，我们可以处理它
                if(inputbuffer.size() > 4) {
                    break;
                }
                // 否则稍微等待一下再尝试
                usleep(10000); // 等待10毫秒
                continue;
            }
            // 其他错误
            qDebug() << "[NetworkClient] Read error:" << strerror(errno);
            connectionClosed = true;
            break;
        }
    }
    
    if(attempts >= maxAttempts) {
        qDebug() << "[NetworkClient] Reached maximum read attempts";
    }
    
    // 如果连接已关闭，处理重连
    if (connectionClosed) {
        qDebug() << "[NetworkClient] Connection closed by server";
        
        // 关闭旧的套接字
        if (socketFd != -1) {
            ::close(socketFd);
            socketFd = -1;
        }
        
        messageWaiting = false;
        
        // 发送连接关闭通知
        emit connectionStateChanged(false);
        
        // 尝试自动重连
        if (autoReconnect) {
            qDebug() << "[NetworkClient] Auto reconnect enabled, attempting to reconnect...";
            if (reconnectToServer()) {
                qDebug() << "[NetworkClient] Reconnected successfully";
                emit connectionStateChanged(true);
                return "{\"response\": \"[系统消息：连接已重建，请重新发送消息]\"}";
            } else {
                qDebug() << "[NetworkClient] Reconnection failed";
                return "{\"response\": \"[系统消息：连接已断开，重连失败，请稍后再试]\"}";
            }
        } else {
            return "{\"response\": \"[系统消息：连接已断开，请重新连接服务器]\"}";
        }
    }
    
    // 检查是否有足够的数据
    if(inputbuffer.size() <= 4) {
        qDebug() << "[NetworkClient] Received message too short:" << inputbuffer.size() << "bytes";
        return "";
    }
    
    // 跳过前4个字节的头部信息，只返回实际的JSON内容
    std::string jsonContent = inputbuffer.substr(4);
    
    return jsonContent;
}

void NetworkClient::disconnectFromServer(){
    if(socketFd == -1) {
        qDebug() << "Socket not initialized";
        return;
    }
    
    messageWaiting = false;
    
    ::close(socketFd);
    socketFd = -1;
}

int NetworkClient::getSocketFd() const {
    return socketFd;
}

// 添加重连方法
bool NetworkClient::reconnectToServer() {
    if (serverIp.empty() || serverPort <= 0) {
        qDebug() << "[NetworkClient] Cannot reconnect, server info not available";
        return false;
    }
    
    // 如果已经连接，先断开
    if (socketFd != -1) {
        disconnectFromServer();
    }
    
    return connectToServer(serverIp, serverPort);
}

