#include <muduo/base/LogFile.h>
#include <muduo/base/Logging.h> // strerror_tl
#include <muduo/base/ProcessInfo.h>

#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace muduo;

// not thread safe
class LogFile::File : boost::noncopyable
{
    public:
        explicit File(const string& filename)
            : fp_(::fopen(filename.data(), "ae")),
            writtenBytes_(0)
        {
            // 创建文件并将缓冲区中的内容写入文件
            assert(fp_);
            ::setbuffer(fp_, buffer_, sizeof buffer_);
            // posix_fadvise POSIX_FADV_DONTNEED ?
        }

        ~File()
        {
            ::fclose(fp_);
        }

        void append(const char* logline, const size_t len)
        {
            size_t n = write(logline, len);
            size_t remain = len - n;
            
            while (remain > 0)
            {
                size_t x = write(logline + n, remain);

                if (x == 0)
                {
                    int err = ferror(fp_);
                    if (err)
                    {
                        fprintf(stderr, "LogFile::File::append() failed %s\n", strerror_tl(err));
                    }
                    break;
                }

                n += x;
                remain = len - n; // remain -= x
            }

            writtenBytes_ += len;
        }

        void flush()
        {
            ::fflush(fp_);
        }

        size_t writtenBytes() const { return writtenBytes_; }

    private:
        size_t write(const char* logline, size_t len)
        {
            #undef fwrite_unlocked
            return ::fwrite_unlocked(logline, 1, len, fp_);
        }

        FILE* fp_;
        char buffer_[64*1024];         
        size_t writtenBytes_;          // 已写入字节数
};

// —————————————————————————————————— LogFile ——————————————————————————————
LogFile::LogFile(const string& basename,
                 size_t rollSize,
                 bool threadSafe,
                 int flushInterval)
    : basename_(basename),
      rollSize_(rollSize),
      flushInterval_(flushInterval),
      count_(0),
      mutex_(threadSafe ? new MutexLock : NULL),
      startOfPeriod_(0),
      lastRoll_(0),
      lastFlush_(0)
{
    // npos是一个常数，表示size_t的最大值（Maximum value for size_t）
    // 许多容器都提供这个东西，用来表示不存在的位置
    assert(basename.find('/') == string::npos);
    rollFile();
}

LogFile::~LogFile()
{
}

void LogFile::append(const char* logline, int len)
{
    if (mutex_)
    {
        MutexLockGuard lock(*mutex_);
        append_unlocked(logline, len);
    }
    else
    {
        append_unlocked(logline, len);
    }
}

void LogFile::flush()
{
    if (mutex_)
    {
        MutexLockGuard lock(*mutex_);
        file_->flush();
    }
    else
    {
        file_->flush();
    }
}

void LogFile::append_unlocked(const char* logline, int len)
{
    // 写入这个文件
    file_->append(logline, len);

    // 当写满 rollSize_ 滚动日志
    if (file_->writtenBytes() > rollSize_)
    {
        rollFile();
    }
    else
    {
        // count_ 是写入文件的次数
        if (count_ > kCheckTimeRoll_)
        {
            count_ = 0;
            time_t now = ::time(NULL);
            // 取整
            time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
            
            // 第二天的 0 点
            if (thisPeriod_ != startOfPeriod_)
            {
                rollFile();
            }
            else if (now - lastFlush_ > flushInterval_)
            {
                // 是否需要flush
                lastFlush_ = now;
                file_->flush();
            }
        }
        else
        {
            ++count_;
        }
    }
}

// 相当于创建新文件，file_ 指针指向新文件
void LogFile::rollFile()
{
    time_t now = 0;
    // 会把获取文件名称的时间返回到 now
    string filename = getLogFileName(basename_, &now);
    // 注意，这里先除 kPollPerSeconds_【一天的秒数】，后乘 kRollPerSeconds_ 表示
    // 对齐至 kRollPerSeconds_ 整数倍，也就是时间调整到当天零点
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    if (now > lastRoll_)
    {
        lastRoll_ = now;
        lastFlush_ = now;

        startOfPeriod_ = start;
        // 产生了一个新的日志文件 new File(filename)
        // p.reset(q) //q为智能指针要指向的新对象
        // 会令智能指针p中存放指针q，即p指向q的空间，而且会释放原来的空间
        file_.reset(new File(filename));
    }
}

// logfile_test.20130411-115604.popo.7743.log
string LogFile::getLogFileName(const string& basename, time_t* now)
{
    string filename;
    // 保留这么大的空间
    filename.reserve(basename.size() + 64);
    filename = basename;

    char timebuf[32];
    char pidbuf[32];
    struct tm tm;
    *now = time(NULL);
    gmtime_r(now, &tm); 
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
    filename += timebuf;
    filename += ProcessInfo::hostname();
    snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
    filename += pidbuf;
    filename += ".log";

    return filename;
}

