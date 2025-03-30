#include"Channel.h"

Channel::Channel(Epoll* ep,int fd):ep_(ep),fd_(fd)
{
}

Channel::~Channel()
{
}

int Channel::fd()
{
    return fd_;
}

void Channel::usset()
{   
    events_ = events_|EPOLLET;  //边缘触发
}                 

void Channel::enablereading()
{
    events_ = events_|EPOLLIN;
    ep_ -> updatechannel(this);
}

void Channel::setinepoll()
{
    inepoll_ = true;
}   

void Channel::setrevents(uint32_t ev)
{
    revents_ = ev;
}

bool Channel::inpoll()
{
    return inepoll_;
}

uint32_t Channel::events()
{
    return events_;
}

uint32_t Channel::revents()
{
    return revents_;
}

void Channel::handleevent()
{
    if(revents_ & EPOLLRDHUP) //对方已关闭
    {                
        printf("client(eventfd=%d)diconnected.\n",fd_);
        close(fd_);                     //关闭客户端id
    }                 
    else if(revents_ & EPOLLIN|EPOLLPRI)  //接收缓冲区有数据可读,IN为读
    {      

        readcallback_();
        /*
        if(islisten_ == true) //如果listenfd有事件连上来，表示有新的客户端连接
        {    
            newconnection(servsock);
        }
        else
        {
            onmessage();
        }
        */
    }
    else if(revents_ & EPOLLOUT)  //有数据需要写，OUT为写
    {             

    }              
    else    //其他情况都视为错误
    {                                           
        printf("client(eventfd=%d)error.\n",fd_);
        close(fd_);
    }                            
}

void Channel::newconnection(Socket* servsock)
{
    InetAddress clientaddr;
    Socket *clientsock = new Socket(servsock -> accept(clientaddr));
    printf("accept client(fd=%d,ip=%s,port=%d) ok.\n",clientsock->fd(),clientaddr.ip(),clientaddr.port());

    Channel *clientchannel = new Channel(ep_,clientsock->fd());
    clientchannel -> setreadcallback(std::bind(&Channel::onmessage,clientchannel));
    clientchannel -> usset();
    clientchannel -> enablereading();
}

void Channel::onmessage()
{
    char buffer[1024];
    while(true) //一次读取buffer数据，直到读取完成
    {    
        bzero(&buffer,sizeof(buffer));
        ssize_t nread = read(fd_,buffer,sizeof(buffer));    //nread代表成功读取到的字节数
        if(nread > 0)//成功读到数据,把接收到的内容发回去
        {     
            printf("recv(eventfd=%d):%s.\n",fd_,buffer);
            send(fd_,buffer,strlen(buffer),0);
        }
        else if(nread == -1 && errno == EINTR)  //读取信号被信号中断，继续读取
        { 
            continue;
        }
        else if(nread == -1 && ((errno == EAGAIN)|| (errno == EWOULDBLOCK)))    //全部数据读取完成
        {   
            break;
        }
        else if(nread == 0) //表示客户端连接断开
        {       
            printf("client(eventfd=%d)disconnected.\n",fd_);
            close(fd_);
            break;
        }
    }
}               

void Channel::setreadcallback(std::function<void()>fn)
{
    readcallback_ = fn;
}
