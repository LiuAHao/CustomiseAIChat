#include "NetworkClient.h"

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<cstdio>
#include<QDebug>       // <-- 将 QDbug 改为 QDebug

NetworkClient::NetworkClient() : socketFd(-1) {}

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
        qDebug() << "Socket not initialized";
        return;
    }

    // --- 恢复之前的发送逻辑 ---
    // 注意：这里的缓冲区大小是固定的，如果消息过长会溢出
    // 并且 strlen 无法正确处理包含'\0'的 std::string
    // 长度也没有转换为网络字节序
    char buffer[1024]; // 使用固定大小缓冲区
    memset(buffer, 0, sizeof(buffer));

    // 使用 strlen 计算长度 (可能不准确)
    int len = strlen(message.c_str());

    // 检查长度是否超出缓冲区（减去4字节长度头）
    if (len + 4 > sizeof(buffer)) {
        qDebug() << "Error: Message too long for fixed buffer!";
        return;
    }

    // 将主机字节序的 int 长度复制到前 4 字节
    memcpy(buffer, &len, 4);

    // 将消息内容复制到长度之后
    memcpy(buffer + 4, message.c_str(), len);

    // 一次性发送 长度 + 内容
    ssize_t bytes_sent = ::send(socketFd, buffer, len + 4, 0);

    // 添加发送结果检查
    if (bytes_sent != len + 4) {
         qDebug() << "Failed to send message completely. Expected:" << (len + 4) << "Sent:" << bytes_sent << "Errno:" << errno;
         // 可以考虑错误处理
    }
 
}

std::string NetworkClient::receiveMessage(){
    if(socketFd == -1) {
        qDebug() << "Socket not initialized";
        return "";
    }

    int32_t net_len = 0; // 用于接收网络字节序的长度
    // 1. 接收 4 字节长度
    ssize_t len_recv = ::recv(socketFd, &net_len, sizeof(net_len), MSG_WAITALL);

    if (len_recv <= 0) {
        if (len_recv == 0) {
             qDebug() << "Connection closed by peer while receiving length.";
        } else if (errno == EINTR) {
            qDebug() << "recv for length interrupted, retrying might be needed or handle error.";
            // 在简单阻塞模型中，EINTR 不常见，但处理一下更健壮
        } else {
            qDebug() << "Failed to receive message length, errno:" << errno;
        }
        return ""; // 返回空字符串表示错误或连接关闭
    }
     if (len_recv != sizeof(net_len)) {
         qDebug() << "Failed to receive full message length, received only" << len_recv << "bytes";
         return "";
     }


    // 2. 将网络字节序转换为 主机字节序
    int32_t len = ntohl(net_len);

    // 3. 验证长度
    if (len <= 0 || len > 1024 * 1024) { // 限制最大长度为 1MB (或根据需要调整)
        qDebug() << "Invalid message length received:" << len;
        // 可以考虑关闭连接或进行其他错误处理
        return "";
    }

    // 4. 准备缓冲区并接收消息体
    std::vector<char> buffer(len); // 使用 vector 动态分配缓冲区
    ssize_t body_recv = ::recv(socketFd, buffer.data(), len, MSG_WAITALL);

    if (body_recv <= 0) {
         if (body_recv == 0) {
             qDebug() << "Connection closed by peer while receiving body.";
         } else if (errno == EINTR) {
             qDebug() << "recv for body interrupted.";
         } else {
             qDebug() << "Failed to receive message body, errno:" << errno;
         }
        return "";
    }
     if (body_recv != len) {
         qDebug() << "Failed to receive full message body, expected" << len << "received" << body_recv;
         return "";
     }


    // 5. 从接收到的数据构造字符串并返回
    return std::string(buffer.data(), len);
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