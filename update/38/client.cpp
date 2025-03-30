//客户端程序
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<time.h>

int main(int argc, char* argv[]){
    if(argc != 3){
        printf("usage:./client ip port\n");
        printf("example:./client 192.168.232.129 5085\n\n");
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

    printf("开始时间：%d.\n",(int)time(0));
    //修改程序为异步发送与接收，在数据包前加入长度信息
    for(int ii = 0; ii < 10; ii++){

        memset(buf,0,sizeof(buf));
        sprintf(buf,"这是第%d段程序",ii);

        char tmpbuf[1024];
        memset(tmpbuf,0,sizeof(tmpbuf));
        int len = strlen(buf);

        memcpy(tmpbuf,&len,4);          //拼接头部信息，第二个为填入的内容，第三个为占据的字节数
        memcpy(tmpbuf + 4,&buf,len);    //拼接内容
        send(sockfd,tmpbuf,len + 4,0);
    
        recv(sockfd,&len,4,0);

        memset(buf,0,sizeof(buf));
        recv(sockfd,buf,len,0);

        //printf("recv:%s.\n",buf);     //终端显示需要时间，不适合性能测试
    }
    printf("结束时间：%d.\n",(int)time(0));
    //  无报文头部
    // for(int ii = 0; ii < 10; ii++){

    //     memset(buf,0,sizeof(buf));
    //     sprintf(buf,"这是第%d段程序",ii);

    //     send(sockfd,buf,strlen(buf),0);

    //     memset(buf,0,sizeof(buf));
    //     recv(sockfd,buf,1024,0);

    //     printf("recv:%s\n",buf);
    //     sleep(1);
    // }

}