// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include <muduo/base/ProcessInfo.h>
#include <muduo/base/FileUtil.h>

#include <algorithm>

#include <assert.h>
#include <dirent.h>
#include <pwd.h>
#include <stdio.h> // snprintf
#include <stdlib.h>
#include <unistd.h>
#include <sys/resource.h>

namespace muduo
{
    namespace detail
    {
        __thread int t_numOpenedFiles = 0;

        /*
          struct dirent
          {
            long d_ino;                 // inode number 索引节点号 
            off_t d_off;                // offset to this dirent 在目录文件中的偏移 
            unsigned short d_reclen;    // length of this d_name 文件名长 
            unsigned char d_type;       // the type of d_name 文件类型 
            char d_name [NAME_MAX+1];   // file name (null-terminated) 文件名，最长255字符 
          }
        */
        int fdDirFilter(const struct dirent* d)
        {
            // isdigit() 用来检测一个字符是否是十进制数字
            if (::isdigit(d->d_name[0]))
            {
                ++t_numOpenedFiles;
            }

            return 0;
        }

        __thread std::vector<pid_t>* t_pids = NULL;
        int taskDirFilter(const struct dirent* d)
        {
            if (::isdigit(d->d_name[0]))
            {
                t_pids->push_back(atoi(d->d_name));
            }

            return 0;
        }

        int scanDir(const char *dirpath, int (*filter)(const struct dirent *))
        {
            struct dirent** namelist = NULL;
            // https://www.cnblogs.com/lidabo/p/5458701.html
            // 就是按照 filter 规则将 dirpath 中的文件过滤到 namelist 中，alphasort 是排序
            // 返回 namelist 中元素的个数
            int result = ::scandir(dirpath, &namelist, filter, alphasort);
            assert(namelist == NULL);
            return result;
        }

        Timestamp g_startTime = Timestamp::now();
    }
}

using namespace muduo;
using namespace muduo::detail;

// 获得进程的 pid
pid_t ProcessInfo::pid()
{
    return ::getpid();
}

// 进程 pid 字符串化
string ProcessInfo::pidString()
{
    char buf[32];
    snprintf(buf, sizeof buf, "%d", pid());
    return buf;
}

// getuid()用来取得执行目前进程的用户识别码
uid_t ProcessInfo::uid()
{
    return ::getuid();
}

/*
获得用户用户名

struct passwd 
{ 
      char *pw_name;       // 用户名
      char *pw_passwd;     // 密码
      __uid_t pw_uid;      // 用户ID
      __gid_t pw_gid;      // 组ID
      char *pw_gecos;      // 真实名
      char *pw_dir;        // 主目录
      char *pw_shell;      // 使用的shell
}; 

以线程安全的方式获取用户名
int getpwuid_r(uid_t uid, struct passwd *pwd, char *buffer,
               size_t bufsize, struct passwd **result);
*/
string ProcessInfo::username()
{
    struct passwd pwd;
    struct passwd* result = NULL;
    char buf[8192];
    const char* name = "unknownuser";

    getpwuid_r(uid(), &pwd, buf, sizeof buf, &result);
    if (result)
    {
        name = pwd.pw_name;
    }
    return name;
}

// 获取用户识别码函数
uid_t ProcessInfo::euid()
{
    return ::geteuid();
}


Timestamp ProcessInfo::startTime()
{
    return g_startTime;     // Timestamp::now()
}

// 得到本机主机名或者域名
string ProcessInfo::hostname()
{
    char buf[64] = "unknownhost";
    buf[sizeof(buf)-1] = '\0';
    ::gethostname(buf, sizeof buf);
    return buf;
}

string ProcessInfo::procStatus()
{
    string result;
    // 从 filename 中读取数据，保存到 content 中
    // filename maxSize content
    // /proc/self/status
    //            包含了所有CPU活跃的信息，该文件中的所有值都是从系统启动开始累计到当前时刻
    FileUtil::readFile("/proc/self/status", 65536, &result);

    return result;
}

int ProcessInfo::openedFiles()
{
    t_numOpenedFiles = 0;
    // /proc/self/fd
    //          从进程角度说正在使用的文件描述符有哪些
    scanDir("/proc/self/fd", fdDirFilter);
    return t_numOpenedFiles;
}

int ProcessInfo::maxOpenFiles()
{
    struct rlimit rl;

    // 获取最大打开文件描述符数限制
    if (::getrlimit(RLIMIT_NOFILE, &rl))
    {
        return openedFiles();
    }
    else
    {
        return static_cast<int>(rl.rlim_cur);
    }
}

int ProcessInfo::numThreads()
{
    int result = 0;
    string status = procStatus();
    size_t pos = status.find("Threads:");

    if (pos != string::npos)
    {
        result = ::atoi(status.c_str() + pos + 8);
    }

    return result;
}

// 获取线程 pid 到 result 中
std::vector<pid_t> ProcessInfo::threads()
{
    std::vector<pid_t> result;
    t_pids = &result;
    scanDir("/proc/self/task", taskDirFilter);
    t_pids = NULL;
    std::sort(result.begin(), result.end());
    return result;
}

