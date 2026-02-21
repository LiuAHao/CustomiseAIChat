all:client tcpepoll bankclient bankserver echoclient

client:client.cpp
	g++ -g -o client client.cpp

bankclient:bankclient.cpp
	g++ -g -o bankclient bankclient.cpp

tcpepoll:tcpepoll.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Acceptor.cpp Connection.cpp Buffer.cpp EchoServer.cpp ThreadPool.cpp Timestamp.cpp
	g++ -g -o tcpepoll tcpepoll.cpp	InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Acceptor.cpp Connection.cpp Buffer.cpp EchoServer.cpp ThreadPool.cpp Timestamp.cpp -lpthread

bankserver:bankserver.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Acceptor.cpp Connection.cpp Buffer.cpp BankServer.cpp ThreadPool.cpp Timestamp.cpp
	g++ -g -o bankserver bankserver.cpp	InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp TcpServer.cpp Acceptor.cpp Connection.cpp Buffer.cpp BankServer.cpp ThreadPool.cpp Timestamp.cpp -lpthread

echoclient:echoclient.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp Connection.cpp Buffer.cpp Connector.cpp TcpClient.cpp Timestamp.cpp
	g++ -g -o echoclient echoclient.cpp InetAddress.cpp Socket.cpp Epoll.cpp Channel.cpp EventLoop.cpp Connection.cpp Buffer.cpp Connector.cpp TcpClient.cpp Timestamp.cpp -lpthread

clean:
	rm -f client tcpepoll bankserver bankclient echoclient