#include <muduo/base/Logging.h>

#include <muduo/base/CurrentThread.h>
#include <muduo/base/StringPiece.h>
#include <muduo/base/Timestamp.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sstream>

namespace muduo
{
    /*
    class LoggerImpl
    {
    public:
      typedef Logger::LogLevel LogLevel;
      LoggerImpl(LogLevel level, int old_errno, const char* file, int line);
      void finish();

      Timestamp time_;
      LogStream stream_;
      LogLevel level_;
      int line_;
      const char* fullname_;
      const char* basename_;
    };
    */

    __thread char t_errnobuf[512];
    __thread char t_time[32];
    __thread time_t t_lastSecond;
    
    // 根据错误码得到对应的错误描述
    // strerror_r 成功返回0  失败返回 -1 并设置errno
    const char* strerror_tl(int savedErrno)
    {
        return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
    }

    // ——————————————————————————————  初始化日志级别 ——————————————————————————————

    Logger::LogLevel initLogLevel()
    {
        return Logger::TRACE;
        /*
        if (::getenv("MUDUO_LOG_TRACE"))
          return Logger::TRACE;
        else if (::getenv("MUDUO_LOG_DEBUG"))
          return Logger::DEBUG;
        else
          return Logger::INFO;
        */
    }

    Logger::LogLevel g_logLevel = initLogLevel();

    // ——————————————————————————————————————————————————————————————————————————————

    // NUM_LOG_LEVELS 在 enum 中定义
    // 每种等级的字符串表示
    const char* LogLevelName[Logger::NUM_LOG_LEVELS] =
    {
        "TRACE ",
        "DEBUG ",
        "INFO  ",
        "WARN  ",
        "ERROR ",
        "FATAL ",
    };

    // helper class for known string length at compile time
    // 判断 str 的长度是否等于 len
    class T
    {
        public:
            T(const char* str, unsigned len)
              :str_(str),
              len_(len)
            {
                assert(strlen(str) == len_);
            }

            const char* str_;
            const unsigned len_;
    };

    // ——————————————————————       重载输出运算符       ———————————————————————

    // 说明在操作符的两端一面是 LogStream 对象，另一个是对象 T
    inline LogStream& operator<<(LogStream& s, T v)
    {
        s.append(v.str_, v.len_);
        return s;
    }

    inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v)
    {
        s.append(v.data_, v.size_);
        return s;
    }

    // —————————————————————— 设置默认的输出函数和刷新函数 ——————————————————————

    void defaultOutput(const char* msg, int len)
    {
        // 默认输出
        /*
            size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)

                参数
                ptr -- 这是指向要被写入的元素数组的指针。
                size -- 这是要被写入的每个元素的大小，以字节为单位。
                nmemb -- 这是元素的个数，每个元素的大小为 size 字节。
                stream -- 这是指向 FILE 对象的指针，该 FILE 对象指定了一个输出流。

                返回值
                如果成功，该函数返回一个 size_t 对象，表示元素的总数，该对象是一个整型数据类型。
                如果该数字与 nmemb 参数不同，则会显示一个错误。
            
            stdout 是标准输出
        */
        size_t n = fwrite(msg, 1, len, stdout);
        //FIXME check n
        (void)n;
    }

    void defaultFlush()
    {
        // 默认刷新缓冲区
        fflush(stdout);
    }

    
    Logger::OutputFunc g_output = defaultOutput;
    Logger::FlushFunc g_flush = defaultFlush;

    // ——————————————————————————————————————————————————————————————————————————
}



using namespace muduo;

// —————————————————————————————————————— Impl ————————————————————————————————————
// 构造函数
Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file, int line)
  : time_(Timestamp::now()),
    stream_(),
    level_(level),
    line_(line),
    basename_(file)
{
    // 格式化时间
    // 【20220314 09:17:10.656961Z】 20630 TRACE main trace ... - Log_test1.cc:8
    formatTime();
    CurrentThread::tid();

    // 20220314 09:17:10.656961Z 【20630 TRACE】 main trace ... - Log_test1.cc:8
    stream_ << T(CurrentThread::tidString(), 6);    // 线程号
    stream_ << T(LogLevelName[level], 6);           // 日志等级的字符串表示

    // 20220314 09:17:10.657161Z 20630 ERROR 【Permission denied (errno=13)】 syserr ... - Log_test1.cc:15
    if (savedErrno != 0)                            // 如果有错误号
    {
        stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
    }
}

// 格式化时间
// 【20220314 09:17:10.656961Z】 20630 TRACE main trace ... - Log_test1.cc:8
void Logger::Impl::formatTime()
{
    // 在 Logger::Impl::time_ 中定义
    int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / 1000000);
    int microseconds = static_cast<int>(microSecondsSinceEpoch % 1000000);

    if (seconds != t_lastSecond)
    {
        t_lastSecond = seconds;
        struct tm tm_time;
        ::gmtime_r(&seconds, &tm_time); // FIXME TimeZone::fromUtcTime

        int len = snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
            tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 17); (void)len;
    }

    Fmt us(".%06dZ ", microseconds);
    assert(us.length() == 9);

    // 将时间输出到 stream 中
    stream_ << T(t_time, 17) << T(us.data(), 9);
}

// 20220314 09:17:10.656961Z 20630 TRACE main trace ... 【- Log_test1.cc:8】
void Logger::Impl::finish()
{
    stream_ << " - " << basename_ << ':' << line_ << '\n';
}

// ————————————————————————————————————————————————————————————————————————————————————

Logger::Logger(SourceFile file, int line)
  : impl_(INFO, 0, file, line)    // 初始化 Impl 的值
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
  : impl_(level, 0, file, line)
{
    // 20220314 09:17:10.657161Z 20630 ERROR Permission denied (errno=13) 【syserr】 ... - Log_test1.cc:15
    impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, LogLevel level)
  : impl_(level, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, bool toAbort)
  : impl_(toAbort?FATAL:ERROR, errno, file, line)
{
}

Logger::~Logger()
{
    impl_.finish();

    const LogStream::Buffer& buf(stream().buffer());
    // 输出缓冲区的数据 Logger::OutputFunc
    g_output(buf.data(), buf.length());

    if (impl_.level_ == FATAL)
    {
        // 刷新缓冲区的函数 
        // Logger::FlushFun
        g_flush();
        // C 库函数 void abort(void) 中止程序执行，直接从调用的地方跳出
        abort();
    }
}

// ————————————————————————————————————————————————————————————————————————————————————

void Logger::setLogLevel(Logger::LogLevel level)
{
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
    g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
    g_flush = flush;
}
