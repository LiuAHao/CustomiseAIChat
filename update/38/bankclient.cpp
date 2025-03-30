//网上银行客户端程序
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<time.h>

//发送报文，四字节报头
ssize_t tcpsend(int fd,void* data,size_t size)
{
    char tmpbuf[1024];
    memset(tmpbuf,0,sizeof(tmpbuf));
    memcpy(tmpbuf,&size,4);
    memcpy(tmpbuf + 4,data,size);
    
    return send(fd,tmpbuf,size + 4,0);
}

//接收报文，四字节报头
ssize_t tcprecv(int fd,void* data)
{
    int len;
    recv(fd,&len,4,0);
    return recv(fd,data,len,0);
}

int main(int argc, char* argv[]){
    if(argc != 3){
        printf("usage:./bankclient ip port\n");
        printf("example:./bankclient 192.168.232.129 5085\n\n");
        return -1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char buf[1024];

    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
        printf("socket() failed.\n");
        return -1;
    }
    
    memset(&servaddr,0,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    if(connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) != 0){
        printf("connect(%s:%s) failed.\n",argv[1],argv[2]);
        close(sockfd);
        return -1;   
    }
    printf("connect ok.\n");

    //登录业务
    memset(buf,0,sizeof(buf));
    sprintf(buf,"<bizcode>00101</bizcode><username>liuahao</username><password>123456</password>");
    if(tcpsend(sockfd,buf,strlen(buf)) <= 0)
    {
        printf("tcpsend() failed\n");
        return -1;
    }
    printf("发送：%s\n",buf);

    memset(buf,0,sizeof(buf));
    if(tcprecv(sockfd,buf) <= 0)
    {
        printf("tcprecv() failed\n");
        return -1;
    }
    printf("接收：%s\n",buf);

    //查询业务
    memset(buf,0,sizeof(buf));
    sprintf(buf,"<bizcode>00201</bizcode>");
    if(tcpsend(sockfd,buf,strlen(buf)) <= 0)
    {
        printf("tcpsend() failed\n");
        return -1;
    }
    printf("发送：%s\n",buf);

    memset(buf,0,sizeof(buf));
    if(tcprecv(sockfd,buf) <= 0)
    {
        printf("tcprecv() failed\n");
        return -1;
    }
    printf("接收：%s\n",buf);


    //注销业务
    memset(buf,0,sizeof(buf));
    sprintf(buf,"<bizcode>00901</bizcode>");
    if(tcpsend(sockfd,buf,strlen(buf)) <= 0)
    {
        printf("tcpsend() failed\n");
        return -1;
    }
    printf("发送：%s\n",buf);

    memset(buf,0,sizeof(buf));
    if(tcprecv(sockfd,buf) <= 0)
    {
        printf("tcprecv() failed\n");
        return -1;
    }
    printf("接收：%s\n",buf);

    while(true)     //每五秒钟发送一次心跳报文，确保连接不会被释放
    {
        memset(buf,0,sizeof(buf));
        sprintf(buf,"<bizcode>00001</bizcode>");
        if(tcpsend(sockfd,buf,strlen(buf)) <= 0)
        {
            printf("tcpsend() failed\n");
            return -1;
        }
        printf("发送：%s\n",buf);
    
        memset(buf,0,sizeof(buf));
        if(tcprecv(sockfd,buf) <= 0)
        {
            printf("tcprecv() failed\n");
            return -1;
        }
        printf("接收：%s\n",buf);
        sleep(5);
    }

}