#include <muduo/base/Logging.h>
#include <muduo/base/LogFile.h>
#include <muduo/base/ThreadPool.h>

#include <stdio.h>

int g_total;
FILE* g_file;
boost::scoped_ptr<muduo::LogFile> g_logFile;

// 最后析构函数会将缓冲区中的数据和函数传给这个函数
void dummyOutput(const char* msg, int len)
{
    g_total += len;

    if (g_file)
    {
        fwrite(msg, 1, len, g_file);
    }
    else if (g_logFile)
    {
        g_logFile->append(msg, len);
    }
}

void bench(const char* type)
{
    // 重新设置输出流的方向
    muduo::Logger::setOutput(dummyOutput);

    muduo::Timestamp start(muduo::Timestamp::now());
    g_total = 0;

    int n = 1000*1000;
    const bool kLongLog = false;
    muduo::string empty = " ";
    muduo::string longStr(3000, 'X');
    longStr += " ";

    for (int i = 0; i < n; ++i)
    {
        LOG_INFO << "Hello 0123456789" << " abcdefghijklmnopqrstuvwxyz"
                << (kLongLog ? longStr : empty)
                << i;
    }

    muduo::Timestamp end(muduo::Timestamp::now());

    double seconds = timeDifference(end, start);
    printf("%12s: %f seconds, %d bytes, %10.2f msg/s, %.2f MiB/s\n",
          type, seconds, g_total, n / seconds, g_total / seconds / (1024 * 1024));
}

void logInThread()
{
    // 线程池当中的线程输出
    LOG_INFO << "logInThread";
    usleep(1000);
}

int main()
{
    getppid(); // 获取父进程 pid

    // 线程池启动了 5 个线程
    muduo::ThreadPool pool("pool");
    pool.start(5);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);
    pool.run(logInThread);

    // 主线程当中的输出，默认是标准输出
    LOG_TRACE << "trace";
    LOG_DEBUG << "debug";
    LOG_INFO << "Hello";
    LOG_WARN << "World";
    LOG_ERROR << "Error";
    LOG_INFO << sizeof(muduo::Logger);
    LOG_INFO << sizeof(muduo::LogStream);
    LOG_INFO << sizeof(muduo::Fmt);
    LOG_INFO << sizeof(muduo::LogStream::Buffer);

    sleep(1);
    // 没有设置 g_file 也没有设置 g_logFile
    bench("nop");

    char buffer[64*1024];

    g_file = fopen("/dev/null", "w");
    // 设置文件流的缓冲区
    setbuffer(g_file, buffer, sizeof buffer);
    // setOutput
    bench("/dev/null");
    fclose(g_file);

    g_file = fopen("/tmp/log", "w");
    setbuffer(g_file, buffer, sizeof buffer);
    bench("/tmp/log");
    fclose(g_file);

    g_file = NULL;
    g_logFile.reset(new muduo::LogFile("test_log_st", 500*1000*1000, false));
    bench("test_log_st");

    g_logFile.reset(new muduo::LogFile("test_log_mt", 500*1000*1000, true));
    bench("test_log_mt");
    g_logFile.reset();
}

/*
20220314 09:33:06.121799Z 21604 TRACE main trace - Logging_test.cc:65
20220314 09:33:06.121974Z 21604 DEBUG main debug - Logging_test.cc:66
20220314 09:33:06.121979Z 21604 INFO  Hello - Logging_test.cc:67
20220314 09:33:06.121982Z 21604 WARN  World - Logging_test.cc:68
20220314 09:33:06.121986Z 21604 ERROR Error - Logging_test.cc:69
20220314 09:33:06.121989Z 21604 INFO  4056 - Logging_test.cc:70
20220314 09:33:06.121992Z 21604 INFO  4024 - Logging_test.cc:71
20220314 09:33:06.121994Z 21604 INFO  36 - Logging_test.cc:72
20220314 09:33:06.121996Z 21604 INFO  4016 - Logging_test.cc:73
20220314 09:33:06.128927Z 21609 INFO  logInThread - Logging_test.cc:49
20220314 09:33:06.128988Z 21605 INFO  logInThread - Logging_test.cc:49
20220314 09:33:06.129033Z 21606 INFO  logInThread - Logging_test.cc:49
20220314 09:33:06.129078Z 21607 INFO  logInThread - Logging_test.cc:49
20220314 09:33:06.129123Z 21608 INFO  logInThread - Logging_test.cc:49
         nop: 0.730602 seconds, 109888890 bytes, 1368734.28 msg/s, 143.44 MiB/s
   /dev/null: 0.767340 seconds, 109888890 bytes, 1303203.27 msg/s, 136.57 MiB/s
    /tmp/log: 1.084850 seconds, 109888890 bytes,  921786.42 msg/s, 96.60 MiB/s
 test_log_st: 3.938204 seconds, 109888890 bytes,  253922.85 msg/s, 26.61 MiB/s
 test_log_mt: 4.862223 seconds, 109888890 bytes,  205667.24 msg/s, 21.55 MiB/s
*/