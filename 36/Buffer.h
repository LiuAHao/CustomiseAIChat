#pragma once
#include<string.h>
#include<iostream>

class Buffer
{
private:
    std::string buf_;
    const uint16_t sep_;        //报文的分隔符： 0无分隔符（固定长度，视频会议），1四字节的报头，2"\r\n\r\n"分隔符（http）协议

public:
    Buffer(const uint16_t sep = 0);     //默认四字节报头
    ~Buffer();
    void append(const char* data,size_t size);      //追加数据到buf_
    void appendwithsep(const char* data,size_t size);  //把数据长度增加到buf_，附加报文分隔符
    void erase(size_t pos,size_t nn);               //在指定位置pos删除nn个数据
    size_t size();                                  //返回buf_大小
    const char* data();                             //返回buf_首地址
    std::string buf();
    void clear();
    bool pickmessage(std::string& ss);              //从buf_中拆分出一个报文，存放在ss中，如果没有，返回false

};