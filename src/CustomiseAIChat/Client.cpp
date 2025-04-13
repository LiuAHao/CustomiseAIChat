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
#include <string>  // 添加string头文件
#include <json/json.h>
#include <QProcess>  // 修改包含方式
#include <QString>   // 修改包含方式
#include <QFile>     // 修改包含方式
#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QFileInfo>
#include <QDebug>   

int main(int argc, char* argv[]){
    if(argc != 3){
        printf("usage:./Client ip port\n");
        printf("example:./Client 192.168.232.129 5085\n\n");
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

    // 修改为使用固定路径或从环境变量获取
    QString program = "/home/liuahao/AIChat/src/ui/build/unknown-Debug/ui";  // 使用绝对路径
    QFileInfo checkFile(program);
    
    if(!checkFile.exists() || !checkFile.isExecutable()) {
        qCritical() << "无效的可执行文件路径或权限不足:" << program;
        return -1;
    }

    // 添加QCoreApplication实例
    QCoreApplication app(argc, argv);
    
    // 使用绝对路径和明确的工作目录
    QProcess *process = new QProcess();
    process->setWorkingDirectory(checkFile.path());
    process->setProgram(program);
    
    // 使用Qt命名空间限定connect函数
    QObject::connect(process, &QProcess::errorOccurred, [](QProcess::ProcessError error){
        qCritical() << "进程启动错误:" << error;
    });
    
    if(!process->startDetached()) {
        qCritical() << "启动失败:" << process->errorString();
        return -1;
    }
    
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("QT_QPA_PLATFORM", "xcb");
    env.insert("DISPLAY", ":0");
    env.insert("QT_DEBUG_PLUGINS", "1");  // 启用插件调试
    
    // 删除重复的process声明，直接使用已创建的process对象
    process->setProcessEnvironment(env);
    
    process->setProgram(program);
    
    // 使用QObject::connect正确连接信号槽
    QObject::connect(process, &QProcess::errorOccurred, [](QProcess::ProcessError error){
        qDebug() << "进程启动错误:" << error;
    });
    
    if(!QFile::exists(program)) {
        printf("错误：找不到Qt前端程序，请检查路径！\n");
    } else if(!process->startDetached()) {
        printf("启动Qt前端失败！%s\n", qPrintable(process->errorString()));
    } else {
        printf("Qt前端已成功启动...\n");
    }

    // 进入Qt前端后，终端客户端可以关闭或保持连接
    printf("Qt前端已启动...\n");

    // printf("连接成功，请输入消息(输入exit退出):\n");
    // while(true) {
    //     printf("你: ");
    //     char input[1024];
    //     fgets(input, sizeof(input), stdin);
        
    //     if(strcmp(input, "exit\n") == 0) break;
        
    //     // 移除末尾换行符
    //     input[strcspn(input, "\n")] = 0;
        
    //     std::string json_msg = "{\"persona_id\":\"p1\",\"messages\":[{\"role\":\"user\",\"content\":\"" 
    //                          + std::string(input) + "\"}]}";
        
    //     // 发送请求
    //     int len = json_msg.size();
    //     char tmpbuf[1024];
    //     memcpy(tmpbuf, &len, 4);
    //     memcpy(tmpbuf + 4, json_msg.c_str(), len);
    //     send(sockfd, tmpbuf, len + 4, 0);
    
    //     // 接收响应
    //     recv(sockfd, &len, 4, 0);
    //     memset(buf, 0, sizeof(buf));
    //     recv(sockfd, buf, len, 0);
    
    //     // 解析并显示AI回复
    //     Json::Value root;
    //     Json::Reader reader;
    //     if(reader.parse(buf, root)) {
    //         printf("AI: %s\n", root["response"].asString().c_str());
    //     } else {
    //         printf("AI回复解析失败\n");
    //     }
    // }

    // //修改程序为异步发送与接收，在数据包前加入长度信息
    // for(int ii = 0; ii < 10; ii++){
    //     // 构建JSON请求
    //     std::string json_msg = "{\"persona_id\":\"p1\",\"messages\":[{\"role\":\"user\",\"content\":\"测试消息\"}]}";
        
    //     int len = json_msg.size();

    //     char tmpbuf[1024];

    //     memcpy(tmpbuf,&len,4);          //拼接头部信息，第二个为填入的内容，第三个为占据的字节数
    //     memcpy(tmpbuf + 4, json_msg.c_str(),len);    //拼接内容
    //     send(sockfd, tmpbuf, len + 4, 0);

    //     recv(sockfd, &len, 4, 0);

    //     memset(buf, 0, sizeof(buf));
    //     recv(sockfd, buf, len, 0);

    //     printf("AI回复:%s.\n",buf);
    // }
    close(sockfd);
    return 0;
}

