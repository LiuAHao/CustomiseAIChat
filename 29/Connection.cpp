#include"Connection.h"


Connection::Connection(EventLoop* loop,Socket* clientsock):loop_(loop),clientsock_(clientsock),disconnect_(false)
{
    clientchannel_ = new Channel(loop_,clientsock_->fd());
    clientchannel_->setreadcallback(std::bind(&Connection::onmessage,this));
    clientchannel_->setclosecallback(std::bind(&Connection::closecallback,this));
    clientchannel_->seterrorcallback(std::bind(&Connection::errorcallback,this));
    clientchannel_->setwritecallback(std::bind(&Connection::writecallback,this));

    clientchannel_->usset();        //边缘触发
    clientchannel_->enablereading();
}

Connection::~Connection()
{
    delete clientsock_;
    delete clientchannel_;
    printf("Connection对象已析构\n");
}

int Connection::fd() const
{
    return clientsock_->fd();
}

std::string Connection::ip()
{
    return clientsock_->ip();
}

uint16_t Connection::port()
{
    return clientsock_->port();
}

void Connection::closecallback()
{   
    disconnect_ = true;
    clientchannel_->remove();
    closecallback_(shared_from_this());
}

void Connection::errorcallback()
{
    disconnect_ = true;
    clientchannel_->remove();
    errorcallback_(shared_from_this());
}

void Connection::writecallback()
{
    int writen = ::send(fd(),outputbuffer_.data(),outputbuffer_.size(),0);  //尝试把全部的outputbuffer_全部发出去
    if(writen > 0)
    {
        outputbuffer_.erase(0,writen);
    }

    if(outputbuffer_.size() == 0)
    {
        clientchannel_->disablewriting();       //如果发送完缓冲区数据，不再关注写事件，避免death loop
        sendcompletecallback_(shared_from_this());            //传输完毕，调用回调函数TcpServer::sendcomplete()
    }
}

void Connection::setclosecallback(std::function<void(spConnection)>fn)
{
    closecallback_ = fn;
}

void Connection::seterrorcallback(std::function<void(spConnection)>fn)
{
    errorcallback_ = fn;
}

void Connection::setonmessagecallback(std::function<void(spConnection,std::string&)>fn)
{
    onmessagecallback_ = fn;
}

void Connection::onmessage()
{
    char buffer[1024];
    while(true) //一次读取buffer数据，直到读取完成
    {    
        bzero(&buffer,sizeof(buffer));
        ssize_t nread = read(fd(),buffer,sizeof(buffer));    //nread代表成功读取到的字节数
        if(nread > 0)//成功读到数据,把接收到的内容发回去
        {     
            inputbuffer_.append(buffer,nread);
        }
        else if(nread == -1 && errno == EINTR)  //读取信号被信号中断，继续读取
        { 
            continue;
        }
        else if(nread == -1 && ((errno == EAGAIN)|| (errno == EWOULDBLOCK)))    //全部数据读取完成
        {   
            while(true)
            {
                //可以把下面的代码封装在Buffer类中
                int len;        //报文长度
                memcpy(&len,inputbuffer_.data(),4);
                if(inputbuffer_.size() < len + 4) break;   //若缓冲区数据量小于头部，说明报文内容不完整

                std::string message(inputbuffer_.data() + 4,len);       //从接收缓冲区中获取一个报文内容
                inputbuffer_.erase(0,len + 4);

                printf("message (eventfd=%d):%s.\n",fd(),message.c_str());
                
                onmessagecallback_(shared_from_this(),message);
            }
            break;
        }
        else if(nread == 0) //表示客户端连接断开
        {       
            clientchannel_->remove();
            closecallback();            
            break;
        }
    }
}  

void Connection::setsendcompletecallback(std::function<void(spConnection)>fn)
{
    sendcompletecallback_ = fn;
}


void Connection::send(const char* data,size_t size)
{
    if(disconnect_ == true)
    {
        printf("连接已经断开，不发送数据\n");
        return;
    }
    outputbuffer_.appendwithhead(data,size);        //把需要加入的数据加入到发送缓存区中
    clientchannel_->enablewriting();        //注册写事件

}
