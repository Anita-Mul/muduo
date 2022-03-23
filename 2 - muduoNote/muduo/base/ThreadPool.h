// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Types.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <deque>

namespace muduo
{
    class ThreadPool : boost::noncopyable
    {
        public:
            // Task 的类型是 void function()
            typedef boost::function<void ()> Task;

            explicit ThreadPool(const string& name = string());
            ~ThreadPool();

            void start(int numThreads);
            void stop();

            void run(const Task& f);                          // 往线程池中的任务队列添加任务

        private:                    
            void runInThread();                               // 线程池中的线程所执行的任务
            Task take();                                      // 获取任务

            MutexLock mutex_;                                 // 任务队列对应的锁
            Condition cond_;                                  // 任务队列对应的条件变量
            string name_;
            boost::ptr_vector<muduo::Thread> threads_;        // 线程池
            std::deque<Task> queue_;                          // 任务队列
            bool running_;                                    // 表示线程池是否处于运行的状态
    };
}

#endif
