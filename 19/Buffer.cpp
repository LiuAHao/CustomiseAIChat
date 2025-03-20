#include"Buffer.h"

// class Buffer
// {
// private:
//     std::string buf_;

// public:
//     Buffer();
//     ~Buffer();
//     void append(const char* data,size_t size);      //追加数据
//     size_t size();                                  //返回buf_大小
//     const char* data();                             //返回buf_首地址
//     void clear();

// };

Buffer::Buffer()
{

}

Buffer::~Buffer()
{

}

void Buffer::append(const char* data,size_t size)
{
    buf_.append(data,size);
}

size_t Buffer::size()
{
    return buf_.size();
}

const char* Buffer::data()
{
    return buf_.data();
}

void Buffer::clear()
{
    buf_.clear();
}
