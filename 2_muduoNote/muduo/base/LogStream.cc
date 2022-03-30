#include <muduo/base/LogStream.h>

#include <algorithm>
#include <limits>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

using namespace muduo;
using namespace muduo::detail;

#pragma GCC diagnostic ignored "-Wtype-limits"
//#pragma GCC diagnostic error "-Wtype-limits"
namespace muduo
{
    namespace detail
    {
        const char digits[] = "9876543210123456789";
        const char* zero = digits + 9;
        BOOST_STATIC_ASSERT(sizeof(digits) == 20);

        const char digitsHex[] = "0123456789ABCDEF";
        BOOST_STATIC_ASSERT(sizeof digitsHex == 17);

        // Efficient Integer to String Conversions, by Matthew Wilson.
        template<typename T>

        // 整数转换成字符串，返回整数的长度，并将转换后的整数字符串存到 buf 中
        // 123 => "123"
        // 十进制调用
        size_t convert(char buf[], T value)
        {
            T i = value;
            char* p = buf;

            do
            {
                // 最后一个数字
                int lsd = static_cast<int>(i % 10);
                i /= 10;
                *p++ = zero[lsd];   // 在上面有定义
            } while (i != 0);

            // 如果是负数
            if (value < 0)
            {
                *p++ = '-';
            }

            *p = '\0';
            // 反转排序容器内指定范围中的元素
            std::reverse(buf, p);

            return p - buf;
        }

        // 十六进制调用
        size_t convertHex(char buf[], uintptr_t value)
        {
            uintptr_t i = value;
            char* p = buf;

            do
            {
                int lsd = i % 16;
                i /= 16;
                *p++ = digitsHex[lsd];
            } while (i != 0);

            *p = '\0';
            std::reverse(buf, p);

            return p - buf;
        }
    }
}

// —————————————————————————————————— FixedBuffer ——————————————————————————————————

template<int SIZE>
const char* FixedBuffer<SIZE>::debugString()
{
    *cur_ = '\0';
    return data_;
}

template<int SIZE>
void FixedBuffer<SIZE>::cookieStart()
{
}

template<int SIZE>
void FixedBuffer<SIZE>::cookieEnd()
{
}

template class FixedBuffer<kSmallBuffer>;
template class FixedBuffer<kLargeBuffer>;

// —————————————————————————————————————————————————————————————————————————————————————

void LogStream::staticCheck()
{
    BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<double>::digits10);
    BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<long double>::digits10);
    BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<long>::digits10);
    BOOST_STATIC_ASSERT(kMaxNumericSize - 10 > std::numeric_limits<long long>::digits10);
}

template<typename T>
// 十进制的数字转换
void LogStream::formatInteger(T v)
{
    // 缓冲区的剩余空间 > 32
    if (buffer_.avail() >= kMaxNumericSize)
    {
        // 把整数转换成字符串存放进去 123 => "123"
        size_t len = convert(buffer_.current(), v);
        // 调整 buffer_ 指针的位置
        buffer_.add(len);
    }
}

LogStream& LogStream::operator<<(short v)
{
    // *this 就是操作符左边的
    // 转换成 int 之后 调用的就是 int 对应的操作符函数
    *this << static_cast<int>(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned short v)
{
    *this << static_cast<unsigned int>(v);
    return *this;
}

LogStream& LogStream::operator<<(int v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned int v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(long long v)
{
    formatInteger(v);
    return *this;
}

LogStream& LogStream::operator<<(unsigned long long v)
{
    formatInteger(v);
    return *this;
}

// 十六进制的数字转换
LogStream& LogStream::operator<<(const void* p)
{
    // uintptr_t 对于 32位平台来说就是 unsigned int
    // 对于 64 位平台来说就是 unsigned long int
    uintptr_t v = reinterpret_cast<uintptr_t>(p);

    if (buffer_.avail() >= kMaxNumericSize)
    {
      char* buf = buffer_.current();
      buf[0] = '0';
      buf[1] = 'x';
      size_t len = convertHex(buf+2, v);
      buffer_.add(len+2);
    }

    return *this;
}

// FIXME: replace this with Grisu3 by Florian Loitsch.
LogStream& LogStream::operator<<(double v)
{
    if (buffer_.avail() >= kMaxNumericSize)
    {
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12g", v);
        buffer_.add(len);
    }

    return *this;
}

// —————————————————————————————————————————————————————————————————————————————————————

// fmt 是格式化字符串的格式
// val 是字符串
template<typename T>
Fmt::Fmt(const char* fmt, T val)
{
    // 断言 T 是算数类型
    BOOST_STATIC_ASSERT(boost::is_arithmetic<T>::value == true);

    // 把 val 按照 fmt 格式，格式化到 buf_ 当中
    length_ = snprintf(buf_, sizeof buf_, fmt, val);

    assert(static_cast<size_t>(length_) < sizeof buf_);
}

// Explicit instantiations

template Fmt::Fmt(const char* fmt, char);

template Fmt::Fmt(const char* fmt, short);
template Fmt::Fmt(const char* fmt, unsigned short);
template Fmt::Fmt(const char* fmt, int);
template Fmt::Fmt(const char* fmt, unsigned int);
template Fmt::Fmt(const char* fmt, long);
template Fmt::Fmt(const char* fmt, unsigned long);
template Fmt::Fmt(const char* fmt, long long);
template Fmt::Fmt(const char* fmt, unsigned long long);

template Fmt::Fmt(const char* fmt, float);
template Fmt::Fmt(const char* fmt, double);