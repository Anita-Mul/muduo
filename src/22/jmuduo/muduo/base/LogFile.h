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

  void append(const char* logline, int len);
  void flush();

 private:
  void append_unlocked(const char* logline, int len);

  static string getLogFileName(const string& basename, time_t* now);
  void rollFile();

  const string basename_;		// ��־�ļ�basename
  const size_t rollSize_;		// ��־�ļ��ﵽrolSize_��һ�����ļ�
  const int flushInterval_;		// ��־д����ʱ��

  int count_;

  boost::scoped_ptr<MutexLock> mutex_;
  time_t startOfPeriod_;	// ��ʼ��¼��־ʱ�䣨����������ʱ�䣩
  time_t lastRoll_;			// ��һ�ι�����־�ļ�ʱ��
  time_t lastFlush_;		// ��һ����־д���ļ�ʱ��
  class File;
  boost::scoped_ptr<File> file_;

  const static int kCheckTimeRoll_ = 1024;
  const static int kRollPerSeconds_ = 60*60*24;
};

}
#endif  // MUDUO_BASE_LOGFILE_H
