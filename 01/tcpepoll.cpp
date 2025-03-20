/*
01版 基础的epoll模型
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

//设置非阻塞IO
void setnonblocking(int fd){
    fcntl(fd,F_SETFL,fcntl(fd,F_GETFL)|O_NONBLOCK);
}

int main(int argc,char *argv[]){
    if(argc != 3){
        printf("usage:./tcpepoll ip port\n");
        printf("example:./tcpepoll 192.168.232.129 5085\n\n");
        return -1;
    }
    //创建服务端监听套接字
    int listenfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(listenfd < 0){
        perror("socket() failed.\n");return -1;
    }

    //设置listenfd的属性
    int opt = 1;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,static_cast<socklen_t>(sizeof opt));   //必须
    setsockopt(listenfd,SOL_SOCKET,TCP_NODELAY,&opt,static_cast<socklen_t>(sizeof opt));    //必须
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEPORT,&opt,static_cast<socklen_t>(sizeof opt));
    setsockopt(listenfd,SOL_SOCKET,SO_KEEPALIVE,&opt,static_cast<socklen_t>(sizeof opt));

    setnonblocking(listenfd);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2]));

    if(bind(listenfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) < 0){
        perror("bind error.\n");
    }

    if(listen(listenfd,128) != 0){
        perror("listen error.\n");
    }

    int epollfd = epoll_create(1);  //创建epoll句柄（红黑树）

    //为服务端listenfd准备读事件
    epoll_event ev; //事件数据结构
    ev.data.fd = listenfd;
    ev.events = EPOLLIN;    //代表可读事件

    epoll_ctl(epollfd,EPOLL_CTL_ADD,listenfd,&ev);  //把listenfd和它的事件加入epollfd队列

    epoll_event evs[10];

    while(true){
        int infds = epoll_wait(epollfd,evs,10,-1);  //等待监视的fd有事件发生
        
        if(infds < 0){
            perror("epoll_wait failed.\n");break;
        }

        if(infds == 0){
            printf("epoll_wait() timeout.\n");
        }
        //如果infds > 0,表示fd发生事件的数量
        for(int ii = 0; ii < infds; ii++){
            if(evs[ii].data.fd == listenfd){    //如果listenfd有事件连上来，表示有新的客户端连接

                struct sockaddr_in clientaddr;
                socklen_t len = sizeof(clientaddr);
                int clientfd = accept(listenfd,(struct sockaddr*)&clientaddr,&len);
                setnonblocking(clientfd);

                printf("accept client(fd=%d,ip=%s,port=%d) ok.\n",clientfd,inet_ntoa(clientaddr.sin_addr),ntohs(clientaddr.sin_port));

                ev.data.fd = clientfd;
                ev.events = EPOLLIN|EPOLLET;    //边缘触发
                epoll_ctl(epollfd,EPOLL_CTL_ADD,clientfd,&ev);

            }
            else{       //如果是客户端连接的fd有事件
                
                if(evs[ii].events & EPOLLRDHUP){                //对方已关闭
                    printf("client(eventfd=%d)diconnected.\n",evs[ii].data.fd);
                    close(evs[ii].data.fd);                     //关闭客户端id
                }                 
                else if(evs[ii].events & EPOLLIN|EPOLLPRI){      //接收缓冲区有数据可读,IN为读
                    char buffer[1024];
                    while(true){    //一次读取buffer数据，直到读取完成
                        bzero(&buffer,sizeof(buffer));
                        ssize_t nread = read(evs[ii].data.fd,buffer,sizeof(buffer));    //nread代表成功读取到的字节数
                        if(nread > 0){     //成功读到数据,
                            //把接收到的内容发回去
                            printf("recv(eventfd=%d):%s.\n",evs[ii].data.fd,buffer);
                            send(evs[ii].data.fd,buffer,strlen(buffer),0);
                        }
                        else if(nread == -1 && errno == EINTR){ //读取信号被信号中断，继续读取
                            continue;
                        }
                        else if(nread == -1 && ((errno == EAGAIN)|| (errno == EWOULDBLOCK))){   //全部数据读取完成
                            break;
                        }
                        else if(nread == 0){    //表示客户端连接断开

                            printf("client(eventfd=%d)disconnected.\n",evs[ii].data.fd);
                            close(evs[ii].data.fd);
                            break;

                        }
                    }
                
                }
                else if(evs[ii].events & EPOLLOUT){             //有数据需要写，OUT为写

                }              
                else{                                           //其他情况都视为错误
                    printf("client(eventfd=%d)error.\n",evs[ii].data.fd);
                    close(evs[ii].data.fd);
                }                                            
                
            }
        }
    }


    return 0;

}