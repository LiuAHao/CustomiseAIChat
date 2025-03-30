/*
06版优化epoll模型
    加入新类Channel,用于封装Channel结构代码,将epoll_event与fd封装
*/

#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<sys/fcntl.h>
#include<sys/epoll.h>
#include<netinet/tcp.h>
#include"InetAddress.h"
#include"Socket.h"
#include"Epoll.h"


int main(int argc,char *argv[]){
    if(argc != 3){
        printf("usage:./tcpepoll ip port\n");
        printf("example:./tcpepoll 192.168.232.129 5085\n\n");
        return -1;
    }
    
    Socket servsock(createnonblocking());
    InetAddress servaddr(argv[1],atoi(argv[2]));
    servsock.setreuseaddr(true); 
    servsock.settcpnodelay(true);
    servsock.setreuseport(true);
    servsock.setkeepalive(true);
   
    servsock.bind(servaddr);
    servsock.listen();

    

    Epoll ep;
    //ep.addfd(servsock.fd(),EPOLLIN);
    Channel *servchannel = new Channel(&ep,servsock.fd());
    servchannel -> enablereading();
    
    while(true)
    {
        
        //std::vector<epoll_event> evs;
        std::vector<Channel*> channels;
        channels = ep.loop();    //等待监视的fd有事件发生
        
        //for(int ii = 0; ii < infds; ii++)
        for(auto &ch:channels)
        {
            if(ch->revents() & EPOLLRDHUP) //对方已关闭
            {                
                printf("client(eventfd=%d)diconnected.\n",ch->fd());
                close(ch->fd());                     //关闭客户端id
            }                 
            else if(ch->revents() & EPOLLIN|EPOLLPRI)  //接收缓冲区有数据可读,IN为读
            {      
                if(ch->fd() == servsock.fd()) //如果listenfd有事件连上来，表示有新的客户端连接
                {    

                    InetAddress clientaddr;
                    Socket *clientsock = new Socket(servsock.accept(clientaddr));
                    
                    printf("accept client(fd=%d,ip=%s,port=%d) ok.\n",clientsock->fd(),clientaddr.ip(),clientaddr.port());
            
                    //ev.addfd(clientsock->fd(),EPOLLIN|EPOLLET); //监听客户端连接的读事件，采用边缘触发
                    Channel *clientchannel = new Channel(&ep,clientsock->fd());
                    clientchannel -> usset();
                    clientchannel -> enablereading();
                }
                else
                {
                    char buffer[1024];
                    while(true) //一次读取buffer数据，直到读取完成
                    {    
                        bzero(&buffer,sizeof(buffer));
                        ssize_t nread = read(ch->fd(),buffer,sizeof(buffer));    //nread代表成功读取到的字节数
                        if(nread > 0)//成功读到数据,把接收到的内容发回去
                        {     
                            printf("recv(eventfd=%d):%s.\n",ch->fd(),buffer);
                            send(ch->fd(),buffer,strlen(buffer),0);
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
                            printf("client(eventfd=%d)disconnected.\n",ch->fd());
                            close(ch->fd());
                            break;
                        }
                    }
                }
            }
            else if(ch->revents() & EPOLLOUT)  //有数据需要写，OUT为写
            {             

            }              
            else    //其他情况都视为错误
            {                                           
                printf("client(eventfd=%d)error.\n",ch->fd());
                close(ch->fd());
            }                                               
        }
    }
    return 0;
}