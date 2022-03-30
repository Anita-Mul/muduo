// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H


// 当创建一个新线程的时候就初始化好里面的数据了
namespace muduo
{
    namespace CurrentThread
    {
        // internal
        extern __thread int t_cachedTid;		        // 线程真实pid（tid）的缓存，
                                                        // 是为了减少::syscall(SYS_gettid)系统调用的次数
                                                        // 提高获取tid的效率
        extern __thread char t_tidString[32];           // 这是tid的字符串表示形式
        extern __thread const char* t_threadName;       // __thread const char* t_threadName = "unknown";
        void cacheTid();

        inline int tid()
        {
            // 说明并没有缓存过
            if (t_cachedTid == 0)
            {
                // 缓存 tid
                cacheTid();
            }

            return t_cachedTid;
        }

        inline const char* tidString()                  // for logging
        {
            return t_tidString;
        }

        inline const char* name()
        {
            return t_threadName;
        }

        bool isMainThread();                            // 判断当前线程的 tid 是否等于进程的 pid（MainThread）
    }     
}

#endif