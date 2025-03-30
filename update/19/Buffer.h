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
    size_t size();                                  //返回buf_大小
    const char* data();                             //返回buf_首地址
    void clear();

};