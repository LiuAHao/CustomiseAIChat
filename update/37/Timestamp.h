#pragma once

#include<iostream>
#include<string>

class Timestamp
{
private:
    time_t secsinceepoch_;      //整数表示的时间（从1970至今的秒数）
public:
    Timestamp();                //当前时间初始化
    Timestamp(int64_t secsinceepoch);   //整数初始化
    ~Timestamp();

    static Timestamp now();     //返回当前时间的Timestamp对象

    time_t toint() const;       //返回整数表示的时间    
    std::string tostring() const;   //返回字符串时间格式： yyyy-mm-dd hh24：mi：ss
};