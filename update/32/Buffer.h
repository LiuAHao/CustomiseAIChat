#pragma once
#include<string>
#include<iostream>

class Buffer
{
private:
    std::string buf_;

public:
    Buffer();
    ~Buffer();
    void append(const char* data,size_t size);      //追加数据
    void appendwithhead(const char* data,size_t size);  //把数据长度怎加到字符串头部
    void erase(size_t pos,size_t nn);               //在指定位置pos删除nn个数据
    size_t size();                                  //返回buf_大小
    const char* data();                             //返回buf_首地址
    std::string buf();
    void clear();

};