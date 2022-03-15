#include <muduo/base/LogStream.h>
#include <muduo/base/Timestamp.h>

#include <sstream>
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using namespace muduo;

const size_t N = 1000000;

#pragma GCC diagnostic ignored "-Wold-style-cast"

template<typename T>
void benchPrintf(const char* fmt)
{
    char buf[32];
    Timestamp start(Timestamp::now());
    for (size_t i = 0; i < N; ++i)
        // 格式化的样式是 fmt，格式化的数字是(T)(i)
        // benchPrintf<int>("%d")
        snprintf(buf, sizeof buf, fmt, (T)(i));
    Timestamp end(Timestamp::now());

    // 计算格式化到普通的 buf 需要的时间
    printf("benchPrintf %f\n", timeDifference(end, start));
}

template<typename T>
void benchStringStream()
{
    Timestamp start(Timestamp::now());
    std::ostringstream os;

    for (size_t i = 0; i < N; ++i)
    {
        os << (T)(i);
        os.seekp(0, std::ios_base::beg);
    }

    Timestamp end(Timestamp::now());

    printf("benchStringStream %f\n", timeDifference(end, start));
}

template<typename T>
void benchLogStream()
{
    Timestamp start(Timestamp::now());
    LogStream os;
    for (size_t i = 0; i < N; ++i)
    {
        os << (T)(i);
        os.resetBuffer();   // cur_ = data_
    }
    Timestamp end(Timestamp::now());

    printf("benchLogStream %f\n", timeDifference(end, start));
}

int main()
{
    benchPrintf<int>("%d");

    puts("int");
    benchPrintf<int>("%d");
    benchStringStream<int>();
    benchLogStream<int>();

    puts("double");
    benchPrintf<double>("%.12g");
    benchStringStream<double>();
    benchLogStream<double>();

    puts("int64_t");
    benchPrintf<int64_t>("%" PRId64);
    benchStringStream<int64_t>();
    benchLogStream<int64_t>();

    puts("void*");
    benchPrintf<void*>("%p");
    benchStringStream<void*>();
    benchLogStream<void*>();
}

/*
benchPrintf 0.106738
int
benchPrintf 0.092209
benchStringStream 0.082551
benchLogStream 0.152786

double
benchPrintf 0.457194
benchStringStream 0.615144
benchLogStream 0.570362

int64_t
benchPrintf 0.092958
benchStringStream 0.052624
benchLogStream 0.099278

void*
benchPrintf 0.085147
benchStringStream 0.055059
benchLogStream 0.075085

*/
