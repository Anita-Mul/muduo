// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CONDITION_H
#define MUDUO_BASE_CONDITION_H

#include <muduo/base/Mutex.h>

#include <boost/noncopyable.hpp>
#include <pthread.h>

namespace muduo
{
    // 是不可以拷贝的
    class Condition : boost::noncopyable
    {
        public:
            explicit Condition(MutexLock& mutex)
              : mutex_(mutex)
            {
                // 初始化条件变量
                pthread_cond_init(&pcond_, NULL);
            }

            ~Condition()
            {
                // 销毁条件变量
                pthread_cond_destroy(&pcond_);
            }

            void wait()
            {
                // 解锁已有的 mutex
                // 待条件满足之后，再加锁 mutex
                pthread_cond_wait(&pcond_, mutex_.getPthreadMutex());
            }

            // returns true if time out, false otherwise.
            bool waitForSeconds(int seconds);

            void notify()
            {
                // 通知阻塞在条件变量上的线程
                pthread_cond_signal(&pcond_);
            }

            void notifyAll()
            {
                pthread_cond_broadcast(&pcond_);
            }

        private:
            MutexLock& mutex_;              // 互斥锁
            pthread_cond_t pcond_;          // 条件变量
    };
}
#endif  // MUDUO_BASE_CONDITION_H
