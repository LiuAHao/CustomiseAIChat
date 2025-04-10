#include"AIServer.h"

AIServer::AIServer(const std::string &ip,const uint16_t port,int subthreadnum,int workthreadnum)
            :tcpserver_(ip,port,subthreadnum),workthreadpool_(workthreadnum,"WORKS"){
    //业务关系什么，就设置什么回调函数
    tcpserver_.setnewconnectioncb(std::bind(&AIServer::HandleNewConnection,this,std::placeholders::_1));
    tcpserver_.setcloseconnectioncb(std::bind(&AIServer::HandleClose,this,std::placeholders::_1));
    tcpserver_.seterrorconnectioncb(std::bind(&AIServer::HandleError,this,std::placeholders::_1)); 
    tcpserver_.setonmessagecb(std::bind(&AIServer::HandleMessage,this,std::placeholders::_1,std::placeholders::_2));
    //tcpserver_.settimeoutcb(std::bind(&EchoServer::HandleEpollTimeOut,this,std::placeholders::_1));
    tcpserver_.setsendcompletecb(std::bind(&AIServer::HandleSendComplete,this,std::placeholders::_1));
   
}

AIServer::~AIServer(){
    workthreadpool_.stop();
    //printf("工作线程已经停止\n");
    tcpserver_.stop();
}

void AIServer::Start(){
    tcpserver_.start();
}

void AIServer::Stop(){

}

void AIServer::HandleNewConnection(spConnection conn){
    printf("%s EchoServer new connection come in(fd=%d,ip=%s,port=%d).\n",Timestamp::now().tostring().c_str(),conn->fd(),conn->ip().data(),conn->port());  
}

void AIServer::HandleClose(spConnection conn){
    printf("%s EchoServer connection closed(fd=%d,ip=%s,port=%d).\n",Timestamp::now().tostring().c_str(),conn->fd(),conn->ip().data(),conn->port());  
}

void AIServer::HandleError(spConnection conn){
    //std::cout<<"EchoServer error connection.\n"; 
}

void AIServer::HandleMessage(spConnection conn,std::string message){
    if(workthreadpool_.size() == 0)         //如果不需要工作线程（工作量小）
    {
        OnMessage(conn,message);
    }
    else
    {
        workthreadpool_.addtask(std::bind(&AIServer::OnMessage,this,conn,message));
    }
   
}

void AIServer::HandleSendComplete(spConnection conn){
} 

//业务处理函数
void AIServer::OnMessage(spConnection conn, std::string message) {
    printf("[客户端消息] %s\n", message.c_str());
    
    // 转义JSON中的特殊字符
    std::string escaped_msg;
    for(char c : message) {
        if(c == '"' || c == '\\') escaped_msg += '\\';
        escaped_msg += c;
    }

    // 构建Python命令
    std::string python_cmd = "PYTHONPATH=/home/liuahao/test1/netserver/python python3 -c \""
                           "from ai_service import process_message; "
                           "print(process_message('" + escaped_msg + "'))\"";
    
    std::string response = ExecPython(python_cmd.c_str(), "");
    
    printf("[AI回应] %s\n", response.c_str());
    conn->send(response.data(), response.size());
}

std::string AIServer::ExecPython(const char* cmd, const char* input){
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"),pclose);
    if(!pipe){
        return "{\"error\":\"popen failed\"}";
    }
    fputs(input,pipe.get());

    while(fgets(buffer.data(),buffer.size(),pipe.get()) != nullptr){
        result += buffer.data();
    }

    return result;
}
