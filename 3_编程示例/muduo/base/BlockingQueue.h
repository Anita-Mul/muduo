// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_BLOCKINGQUEUE_H
#define MUDUO_BASE_BLOCKINGQUEUE_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <boost/noncopyable.hpp>
#include <deque>
#include <assert.h>

namespace muduo
{
    template<typename T>
    class BlockingQueue : boost::noncopyable
    {
        public:
            BlockingQueue()
              : mutex_(),
                notEmpty_(mutex_),
                queue_()
            {
            }

            void put(const T& x)
            {
                MutexLockGuard lock(mutex_);
                queue_.push_back(x);
                notEmpty_.notify(); // TODO: move outside of lock
            }

            T take()
            {
                MutexLockGuard lock(mutex_);
                // always use a while-loop, due to spurious wakeup
                while (queue_.empty())
                {
                    notEmpty_.wait();
                }
                
                assert(!queue_.empty());
                T front(queue_.front());
                queue_.pop_front();
                return front;
            }

            size_t size() const
            {
                MutexLockGuard lock(mutex_);
                return queue_.size();
            }

        private:
            // 可以改变 mutable 修饰成员的状态
            mutable MutexLock mutex_;                   // 锁
            Condition         notEmpty_;                // 条件变量
            std::deque<T>     queue_;                   // 队列
    };
}

#endif  // MUDUO_BASE_BLOCKINGQUEUE_H
