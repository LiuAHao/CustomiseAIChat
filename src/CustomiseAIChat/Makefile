# 编译器设置
CXX = g++
# 修改编译器设置
CXXFLAGS = -g -std=c++11 -I../net -I/usr/include/jsoncpp \
           -I/usr/include/x86_64-linux-gnu/qt5 \
           -I/usr/include/x86_64-linux-gnu/qt5/QtCore \
           -I/usr/include/x86_64-linux-gnu/qt5/QtGui \
           -I/usr/include/x86_64-linux-gnu/qt5/QtWidgets \
           -fPIC

# 修改链接选项
LDFLAGS = -lpthread -ljsoncpp -lQt5Core -lQt5Gui -lQt5Widgets -lQt5XcbQpa -lxcb-cursor

# 目标文件 - 移除Client
TARGETS = Server

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

# 清理规则
clean:
	rm -f $(TARGETS)

.PHONY: all clean

# 添加Qt核心模块
QT_CORE = -lQt5Core

# 修改链接选项
LDFLAGS = -lpthread -ljsoncpp -lQt5Core -lQt5Gui -lQt5Widgets -lQt5XcbQpa -lxcb-cursor
LIBS = $(QT_CORE) $(QT_NETWORK) -ljsoncpp

# 添加Qt头文件路径
INCLUDES = -I/usr/include/qt5 -I/usr/include/qt5/QtCore