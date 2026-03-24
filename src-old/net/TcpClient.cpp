#include"TcpClient.h"

TcpClient::TcpClient(const std::string &ip,uint16_t port)
            :loop_(true),connector_(&loop_,ip,port)
{
    //设置epoll_wait超时回调（必须设置，否则EventLoop::run()中会崩溃）
    loop_.setepolltimeoutcallback([](EventLoop*){});

    //设置Connector的回调函数
    connector_.setnewconnectioncb(std::bind(&TcpClient::newconnection,this,std::placeholders::_1));
    connector_.seterrorconnectioncb(std::bind(&TcpClient::connecterror,this));
}

TcpClient::~TcpClient()
{
}

void TcpClient::start()
{
    connector_.start();     //发起非阻塞连接
    loop_.run();            //启动事件循环（阻塞）
}

void TcpClient::stop()
{
    loop_.stop();
}

bool TcpClient::send(const char* data,size_t size)
{
    if(conn_ == nullptr) return false;
    conn_->send(data,size);
    return true;
}

spConnection TcpClient::connection()
{
    return conn_;
}

void TcpClient::newconnection(std::unique_ptr<Socket> clientsock)
{
    //连接成功，创建Connection对象（复用服务端同一个Connection类）
    conn_.reset(new Connection(&loop_,std::move(clientsock)));

    //设置Connection的回调函数（回调链：Channel → Connection → TcpClient → 用户回调）
    conn_->setclosecallback(std::bind(&TcpClient::closeconnection,this,std::placeholders::_1));
    conn_->seterrorcallback(std::bind(&TcpClient::errorconnection,this,std::placeholders::_1));
    conn_->setonmessagecallback(std::bind(&TcpClient::onmessage,this,std::placeholders::_1,std::placeholders::_2));
    conn_->setsendcompletecallback(std::bind(&TcpClient::sendcomplete,this,std::placeholders::_1));

    if(connectioncb_) connectioncb_(conn_);
}

void TcpClient::closeconnection(spConnection conn)
{
    if(closecb_) closecb_(conn);
    conn_.reset();
}

void TcpClient::errorconnection(spConnection conn)
{
    if(errorcb_) errorcb_(conn);
    conn_.reset();
}

void TcpClient::onmessage(spConnection conn,std::string& message)
{
    if(messagecb_) messagecb_(conn,message);
}

void TcpClient::sendcomplete(spConnection conn)
{
    if(sendcompletecb_) sendcompletecb_(conn);
}

void TcpClient::connecterror()
{
    printf("TcpClient connect failed.\n");
    stop();
}

void TcpClient::setconnectioncb(std::function<void(spConnection)> fn)
{
    connectioncb_ = fn;
}

void TcpClient::setclosecb(std::function<void(spConnection)> fn)
{
    closecb_ = fn;
}

void TcpClient::seterrorcb(std::function<void(spConnection)> fn)
{
    errorcb_ = fn;
}

void TcpClient::setmessagecb(std::function<void(spConnection,std::string&)> fn)
{
    messagecb_ = fn;
}

void TcpClient::setsendcompletecb(std::function<void(spConnection)> fn)
{
    sendcompletecb_ = fn;
}
