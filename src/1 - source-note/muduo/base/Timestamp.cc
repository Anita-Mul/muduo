#include <muduo/base/Timestamp.h>

#include <sys/time.h>
#include <stdio.h>

// 先包含进来 inttypes.h 这个宏
// 之后取消包含
#define __STDC_FORMAT_MACROS
    #include <inttypes.h>
#undef __STDC_FORMAT_MACROS

#include <boost/static_assert.hpp>

using namespace muduo;

// Boost中提供了一个编译时断言宏 BOOST_STATIC_ASSERT
// assert() 是运行时断言
BOOST_STATIC_ASSERT(sizeof(Timestamp) == sizeof(int64_t));


Timestamp::Timestamp(int64_t microseconds)
  : microSecondsSinceEpoch_(microseconds)
{
}


string Timestamp::toString() const
{
    char buf[32] = {0};
    int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
    int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;

    // int snprintf(char *str, int n, char * format [, argument, ...]);
    // str为要写入的字符串；n为要写入的字符的最大数目
    // PRId64
    snprintf(buf, sizeof(buf)-1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
    return buf;
}


string Timestamp::toFormattedString() const
{
    char buf[32] = {0};
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(microSecondsSinceEpoch_ % kMicroSecondsPerSecond);
    
    // https://www.cnblogs.com/raceblog/archive/2009/07/13/1522779.html#:~:text=tm%E6%98%AFC%E8%AF%AD%E8%A8%80%E4%B8%AD%E5%AE%9A,%E9%9D%A2%E6%98%AF%E8%AF%A6%E7%BB%86%E7%9A%84%E5%AE%9A%E4%B9%89%EF%BC%9A
    struct tm tm_time;
    // 转换为 GMT 时间，加上 _r 是一个线程安全的函数
    gmtime_r(&seconds, &tm_time);

    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
        tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
        tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
        microseconds);
    return buf;
}

// 返回距离 1970-01-01 00:00:00 所流过的微秒数
Timestamp Timestamp::now()
{
    // tv.tv_sec 秒
    // tv.tv_usec 微秒
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t seconds = tv.tv_sec; 
    return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

Timestamp Timestamp::invalid()
{
    return Timestamp();
}

