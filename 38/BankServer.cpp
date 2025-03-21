#include"BankServer.h"

BankServer::BankServer(const std::string &ip,const uint16_t port,int subthreadnum,int workthreadnum)
            :tcpserver_(ip,port,subthreadnum),workthreadpool_(workthreadnum,"WORKS")
{
    //业务关系什么，就设置什么回调函数
    tcpserver_.setnewconnectioncb(std::bind(&BankServer::HandleNewConnection,this,std::placeholders::_1));
    tcpserver_.setcloseconnectioncb(std::bind(&BankServer::HandleClose,this,std::placeholders::_1));
    tcpserver_.seterrorconnectioncb(std::bind(&BankServer::HandleError,this,std::placeholders::_1)); 
    tcpserver_.setonmessagecb(std::bind(&BankServer::HandleMessage,this,std::placeholders::_1,std::placeholders::_2));
    //tcpserver_.settimeoutcb(std::bind(&BankServer::HandleEpollTimeOut,this,std::placeholders::_1));
    tcpserver_.setsendcompletecb(std::bind(&BankServer::HandleSendComplete,this,std::placeholders::_1));
    tcpserver_.setremoveconnectioncb(std::bind(&BankServer::HandleRemove,this,std::placeholders::_1));


}

BankServer::~BankServer()
{
    workthreadpool_.stop();
    //printf("工作线程已经停止\n");
    tcpserver_.stop();
}

void BankServer::Start()
{
    tcpserver_.start();
}

void BankServer::Stop()
{

}


void BankServer::HandleNewConnection(spConnection conn)
{
    printf("%s EchoServer new connection come in(fd=%d,ip=%s,port=%d).\n",Timestamp::now().tostring().c_str(),conn->fd(),conn->ip().data(),conn->port());  

    spUserInfo userinfo(new UserInfo(conn->fd(),conn->ip()));
    {
        std::lock_guard<std::mutex> gd(mutex_);
        usermap_[conn->fd()] = userinfo;
    }
    printf("%s 新建连接（ip = %s）\n",Timestamp::now().tostring().c_str(),conn->ip().c_str());
}

void BankServer::HandleClose(spConnection conn)
{
    printf("%s 连接断开（ip = %s）\n",Timestamp::now().tostring().c_str(),conn->ip().c_str());

    {
        std::lock_guard<std::mutex> gd(mutex_);
        usermap_.erase(conn->fd());
    }
}

void BankServer::HandleError(spConnection conn)
{
     HandleClose(conn);
}

void BankServer::HandleMessage(spConnection conn,std::string message)
{
    if(workthreadpool_.size() == 0)         //如果不需要工作线程（工作量小）
    {
        OnMessage(conn,message);
    }
    else
    {
        workthreadpool_.addtask(std::bind(&BankServer::OnMessage,this,conn,message));
    }
   
}

void BankServer::HandleSendComplete(spConnection conn)
{
    //std::cout<<"EchoServer send complete.\n"; 

} 

// void EchoServer::HandleEpollTimeOut(EventLoop* loop)
// {
//     std::cout<<"EchoServer timeout.\n"; 

// }

bool getxmlbuffer(const std::string& xmlbuffer,const std::string& fieldname,std::string& value,const int ilen = 0)
{
    std::string start = "<" + fieldname + ">";  //数据项开始标签
    std::string end = "</" + fieldname + ">";   //数据项结束标签

    int startp = xmlbuffer.find(start);
    if(startp == std::string::npos)return false;

    //从xml中截取数据项内容
    int endp = xmlbuffer.find(end);
    if(endp == std::string::npos)return false;

    int itmplen = endp - startp - start.length();
    if((ilen > 0)&&(ilen < itmplen))itmplen = ilen; //ilen代表需要在数据项中截取的长度，如果没有则截取全部
    value = xmlbuffer.substr(startp + start.length(),itmplen);

    return true;
}

void BankServer::OnMessage(spConnection conn,std::string message)
{
    spUserInfo userinfo = usermap_[conn->fd()];
    //解析客户端请求报文
    std::string bizcode;        //业务代码
    std::string replaymessage;  //回应报文

    getxmlbuffer(message,"bizcode",bizcode);    //从请求报文中解析出业务代码

    if(bizcode == "00101")      //登录业务
    {
        std::string username,password;
        getxmlbuffer(message,"username",username);
        getxmlbuffer(message,"password",password);
        if(username == "liuahao" && password == "123456")   //实际开发中应该从数据库或redis查询
        {
            replaymessage = "<bizcode>00102</bizcode><retcode>0</retcode><message>登陆成功。</message>";
            userinfo->setLogin(true);
        }
        else
        {
            replaymessage = "<bizcode>00102</bizcode><retcode>-1</retcode><message>用户名或密码不正确。</message>";
        }
    }
    else if(bizcode == "00201")
    {
        if(userinfo->Login() == true)
        {
            replaymessage = "<bizcode>00202</bizcode><retcode>0</retcode><message>666</message>";   //发送账户余额
        }
        else
        {
            replaymessage = "<bizcode>00202</bizcode><retcode>-1</retcode><message>账户未登录。</message>";   //发送账户余额
        }
    }
    else if(bizcode == "00901")
    {
        if(userinfo->Login() == true)
        {
            replaymessage = "<bizcode>00902</bizcode><retcode>0</retcode><message>注销成功。</message>"; 
            userinfo->setLogin(false);
        }
        else
        {
            replaymessage = "<bizcode>00902</bizcode><retcode>-1</retcode><message>账户未登录。</message>"; 
        }
    }
    else if(bizcode == "00001")     //增加心跳业务
    {
        if(userinfo->Login() == true)
        {
            replaymessage = "<bizcode>00002</bizcode><retcode>0</retcode><message>心跳：保持连接</message>"; 
            userinfo->setLogin(false);
        }
        else
        {
            replaymessage = "<bizcode>00002</bizcode><retcode>-1</retcode><message>账户未登录。</message>"; 
        }
    }
    
    conn->send(replaymessage.data(),replaymessage.size());
}

void BankServer::HandleRemove(int fd)
{
    printf("fd(%d)已超时\n",fd);
    usermap_.erase(fd);
}
