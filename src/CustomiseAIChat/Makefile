# 编译器设置
CXX = g++
CXXFLAGS = -g -std=c++11 -I../net -I/usr/include/jsoncpp
LDFLAGS = -lpthread -ljsoncpp

# 目标文件
TARGETS = Server Client

# 默认目标
all: $(TARGETS)

# 源文件
NET_SRCS = \
    ../net/TcpServer.cpp \
    ../net/InetAddress.cpp \
    ../net/Socket.cpp \
    ../net/Epoll.cpp \
    ../net/Channel.cpp \
    ../net/EventLoop.cpp \
    ../net/Acceptor.cpp \
    ../net/Connection.cpp \
    ../net/Buffer.cpp \
    ../net/ThreadPool.cpp \
    ../net/Timestamp.cpp

# Server编译规则
Server: Server.cpp AIServer.cpp $(NET_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Client编译规则
Client: Client.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)  # Added $(LDFLAGS) here

# 清理规则
clean:
	rm -f $(TARGETS)

.PHONY: all clean