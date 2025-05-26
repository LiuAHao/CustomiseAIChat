#include"AIServer.h"
#include<curl/curl.h>  // 添加curl库头文件

AIServer::AIServer(const std::string &ip,const uint16_t port,int subthreadnum,int workthreadnum)
            :tcpserver_(ip,port,subthreadnum),workthreadpool_(workthreadnum,"WORKS"){
    //业务关系什么，就设置什么回调函数
    tcpserver_.setnewconnectioncb(std::bind(&AIServer::HandleNewConnection,this,std::placeholders::_1));
    tcpserver_.setcloseconnectioncb(std::bind(&AIServer::HandleClose,this,std::placeholders::_1));
    tcpserver_.seterrorconnectioncb(std::bind(&AIServer::HandleError,this,std::placeholders::_1)); 
    tcpserver_.setonmessagecb(std::bind(&AIServer::HandleMessage,this,std::placeholders::_1,std::placeholders::_2));
    //tcpserver_.settimeoutcb(std::bind(&EchoServer::HandleEpollTimeOut,this,std::placeholders::_1));
    tcpserver_.setsendcompletecb(std::bind(&AIServer::HandleSendComplete,this,std::placeholders::_1));
    tcpserver_.setremoveconnectioncb(std::bind(&AIServer::HandleRemove,this,std::placeholders::_1));

    // 初始化CURL
    curl_global_init(CURL_GLOBAL_ALL);
}

AIServer::~AIServer(){
    workthreadpool_.stop();
    tcpserver_.stop();
    
    // 清理CURL
    curl_global_cleanup();
}

void AIServer::Start(){
    tcpserver_.start();
}

void AIServer::Stop(){

}

void AIServer::HandleNewConnection(spConnection conn){
    printf("%s new connection come in(fd=%d,ip=%s,port=%d).\n",Timestamp::now().tostring().c_str(),conn->fd(),conn->ip().data(),conn->port());  
}

void AIServer::HandleClose(spConnection conn){
    printf("%s connection closed(fd=%d,ip=%s,port=%d).\n",Timestamp::now().tostring().c_str(),conn->fd(),conn->ip().data(),conn->port());  
}

void AIServer::HandleError(spConnection conn){
    std::cout<<"EchoServer error connection.\n"; 
}

void AIServer::HandleMessage(spConnection conn,std::string message){
    if(workthreadpool_.size() == 0)         // 如果不需要工作线程（工作量小）
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

void AIServer::HandleRemove(int fd){
    printf("fd(%d)已超时\n",fd);
}

void AIServer::Base64Encode(const std::string& input, std::string* output) {
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    int i = 0, j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    const auto* bytes_to_encode = reinterpret_cast<const unsigned char*>(input.data());
    size_t in_len = input.size();

    output->clear();
    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; i <4 ; i++)
                *output += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; j < i + 1; j++)
            *output += base64_chars[char_array_4[j]];

        while((i++ < 3))
            *output += '=';
    }
}

// CURL回调函数：用于接收HTTP响应
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// 发送HTTP请求到Python服务
std::string AIServer::SendHttpRequest(const std::string& base64_msg) {
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(!curl) {
        return "{\"error\":\"无法初始化CURL\"}";
    }

    // 设置请求URL（Python服务的地址）
    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:5001/process");
    
    // URL编码消息内容
    char* encoded_message = curl_easy_escape(curl, base64_msg.c_str(), base64_msg.length());
    if (!encoded_message) {
        curl_easy_cleanup(curl);
        return "{\"error\":\"URL编码失败\"}";
    }
    
    // 构造POST数据
    std::string postFields = "message=" + std::string(encoded_message);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
    
    // 设置回调函数
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    
    // 执行请求
    res = curl_easy_perform(curl);
    
    // 释放编码后的字符串
    curl_free(encoded_message);
    
    // 检查是否有错误
    if(res != CURLE_OK) {
        std::string error = "{\"error\":\"curl_easy_perform() 失败: " + std::string(curl_easy_strerror(res)) + "\"}";
        curl_easy_cleanup(curl);
        return error;
    }
    
    // 清理
    curl_easy_cleanup(curl);
    
    return readBuffer;
}

//业务处理函数
void AIServer::OnMessage(spConnection conn, std::string message) {
    printf("[客户端消息] %s\n", message.c_str());

    // Base64编码消息
    std::string base64_msg;
    AIServer::Base64Encode(message, &base64_msg);
    // 发送HTTP请求到Python服务
    std::string response = SendHttpRequest(base64_msg);
    printf("[AI回应] %s\n", response.c_str());
    // 发送响应给客户端
    conn->send(response.data(), response.size());
}