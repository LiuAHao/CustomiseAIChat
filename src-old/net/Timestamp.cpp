#include"Timestamp.h"

Timestamp::Timestamp()
{
    secsinceepoch_ = time(0);
}

Timestamp::Timestamp(int64_t secsinceepoch):secsinceepoch_(secsinceepoch)
{

}


Timestamp::~Timestamp()
{

}

Timestamp Timestamp::now()
{
    return Timestamp();     //返回当前时间
}

time_t Timestamp::toint() const
{
    return secsinceepoch_;
}
std::string Timestamp::tostring() const
{
    char buf[128] = {0};
    tm* tm_time = localtime(&secsinceepoch_);       //int转换格式化时间类型
    snprintf(buf,128,"%4d-%02d-%02d %02d:%02d:%02d",
        tm_time->tm_year + 1900,
        tm_time->tm_mon + 1,
        tm_time->tm_mday,
        tm_time->tm_hour,
        tm_time->tm_min,
        tm_time->tm_sec
    );                  //编辑字符串格式
    return buf;
}
// #include<unistd.h>
// int main()
// {
//     Timestamp ts;
//     std::cout<<ts.tostring()<<std::endl;
//     sleep(1);
//     std::cout<<Timestamp::now().tostring()<<std::endl;

// }
//g++ -o test Timestamp.cpp