#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include <muduo/base/Mutex.h>
#include <muduo/base/Types.h>

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

namespace muduo
{
        class LogFile : boost::noncopyable
        {
                public:
                        LogFile(const string& basename,
                                size_t rollSize,
                                bool threadSafe = true,
                                int flushInterval = 3);
                        ~LogFile();
                        
                        // 添加到日志文件当中
                        void append(const char* logline, int len);
                        // 清空缓冲区
                        void flush();

                private:
                        // 不加锁的方式添加
                        void append_unlocked(const char* logline, int len);

                        static string getLogFileName(const string& basename, time_t* now);
                        // 滚动日志
                        void rollFile();

                        const string basename_;		// 日志文件的 basename
                        const size_t rollSize_;		// 日志文件达到 rolSize 换一个新文件
                        const int flushInterval_;       // 日志写入间隔时间

                        int count_;

                        boost::scoped_ptr<MutexLock> mutex_;
                        time_t startOfPeriod_;	        // 开始记录日志时间（调整至零点的时间）
                        time_t lastRoll_;		// 上一次滚动日志文件时间
                        time_t lastFlush_;		// 上一次日志写入文件时间
                        class File;                     // 自定义文件类
                        //  boost::scoped_ptr 智能指针，它能够保证在离开作用域后对象被自动释放
                        boost::scoped_ptr<File> file_;

                        const static int kCheckTimeRoll_ = 1024;
                        const static int kRollPerSeconds_ = 60*60*24;
        };
}
#endif  // MUDUO_BASE_LOGFILE_H
