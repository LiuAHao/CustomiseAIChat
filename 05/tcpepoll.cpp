/*
05版优化epoll模型
    加入新类Epoll,用于封装Epoll结构代码
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

    // int epollfd = epoll_create(1);  //创建epoll句柄（红黑树）

    // //为服务端listenfd准备读事件
    // epoll_event ev; //事件数据结构
    // ev.data.fd = servsock.fd();
    // ev.events = EPOLLIN;    //代表可读事件

    // epoll_ctl(epollfd,EPOLL_CTL_ADD,servsock.fd(),&ev);  //把listenfd和它的事件加入epollfd队列

    // epoll_event evs[10];

    Epoll ep;
    ep.addfd(servsock.fd(),EPOLLIN);
    std::vector<epoll_event> evs;
    
    while(true)
    {
        // int infds = epoll_wait(epollfd,evs,10,-1);  //等待监视的fd有事件发生
        
        // if(infds < 0)//如果infds > 0,表示fd发生事件的数量
        // {
        //     perror("epoll_wait failed.\n");break;
        // }
        // if(infds == 0)
        // {
        //     printf("epoll_wait() timeout.\n");
        // }

        evs = ep.loop();    //等待监视的fd有事件发生
        
        //for(int ii = 0; ii < infds; ii++)
        for(auto &ev:evs)
        {
            if(ev.events & EPOLLRDHUP) //对方已关闭
            {                
                printf("client(eventfd=%d)diconnected.\n",ev.data.fd);
                close(ev.data.fd);                     //关闭客户端id
            }                 
            else if(ev.events & EPOLLIN|EPOLLPRI)  //接收缓冲区有数据可读,IN为读
            {      
                if(ev.data.fd == servsock.fd()) //如果listenfd有事件连上来，表示有新的客户端连接
                {    

                  
                    InetAddress clientaddr;
                    Socket *clientsock = new Socket(servsock.accept(clientaddr));
                    
                    printf("accept client(fd=%d,ip=%s,port=%d) ok.\n",clientsock->fd(),clientaddr.ip(),clientaddr.port());
            
                    //ev.data.fd = clientsock->fd();
                    //ev.events = EPOLLIN|EPOLLET;    //读事件与边缘触发
                    //epoll_ctl(epollfd,EPOLL_CTL_ADD,clientsock->fd(),&ev);
                    ep.addfd(clientsock->fd(),EPOLLIN|EPOLLET); //监听客户端连接的读事件，采用边缘触发
                }
                else
                {
                    char buffer[1024];
                    while(true) //一次读取buffer数据，直到读取完成
                    {    
                        bzero(&buffer,sizeof(buffer));
                        ssize_t nread = read(ev.data.fd,buffer,sizeof(buffer));    //nread代表成功读取到的字节数
                        if(nread > 0)//成功读到数据,把接收到的内容发回去
                        {     
                            printf("recv(eventfd=%d):%s.\n",ev.data.fd,buffer);
                            send(ev.data.fd,buffer,strlen(buffer),0);
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
                            printf("client(eventfd=%d)disconnected.\n",ev.data.fd);
                            close(ev.data.fd);
                            break;
                        }
                    }
                }
            }
            else if(ev.events & EPOLLOUT)  //有数据需要写，OUT为写
            {             

            }              
            else    //其他情况都视为错误
            {                                           
                printf("client(eventfd=%d)error.\n",ev.data.fd);
                close(ev.data.fd);
            }                                               
        }
    }
    return 0;
}