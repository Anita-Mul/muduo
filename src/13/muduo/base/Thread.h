// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H

#include <muduo/base/Atomic.h>
#include <muduo/base/Types.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{
    class Thread : boost::noncopyable
    {
        public:
            typedef boost::function<void ()> ThreadFunc;

            // string() 空的字符串类
            explicit Thread(const ThreadFunc&, const string& name = string());
            ~Thread();


            void start();   // 启动一个线程
            int join();     // return pthread_join()


            // 返回线程有关信息
            bool started() const { return started_; }                   // 线程是否已经启动
            // pthread_t pthreadId() const { return pthreadId_; }       
            pid_t tid() const { return tid_; }
            const string& name() const { return name_; }


            static int numCreated() { return numCreated_.get(); }       // 返回已经创建线程的个数

        private:
            static void* startThread(void* thread);         // 线程的入口函数
            void runInThread();                             

            bool       started_;                // 表示线程是否启动
            pthread_t  pthreadId_;      
            pid_t      tid_;
            ThreadFunc func_;
            string     name_;

            static AtomicInt32 numCreated_;     // 已经创建线程的个数
    };
}
#endif
