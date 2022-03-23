// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_MUTEX_H
#define MUDUO_BASE_MUTEX_H

#include <muduo/base/CurrentThread.h>
#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

namespace muduo
{
    // 是不可以拷贝的
    class MutexLock : boost::noncopyable
    {
        public:
            MutexLock()
              : holder_(0)
            {
                // 初始化锁
                int ret = pthread_mutex_init(&mutex_, NULL);
                assert(ret == 0); (void) ret;
            }

            ~MutexLock()
            {
                assert(holder_ == 0);
                // 销毁锁
                int ret = pthread_mutex_destroy(&mutex_);
                assert(ret == 0); (void) ret;
            }

            bool isLockedByThisThread()
            {
                return holder_ == CurrentThread::tid();
            }

            void assertLocked()
            {
                assert(isLockedByThisThread());
            }

            // internal usage

            void lock()
            {
                pthread_mutex_lock(&mutex_);
                holder_ = CurrentThread::tid();
            }

            void unlock()
            {
                holder_ = 0;
                pthread_mutex_unlock(&mutex_);
            }

            pthread_mutex_t* getPthreadMutex() /* non-const */
            {
                return &mutex_;
            }

        private:
            pthread_mutex_t mutex_;     // 互斥锁
            pid_t holder_;              // 持有者  线程 pid
    };

    // 使用 RAII 技法封装
    class MutexLockGuard : boost::noncopyable
    {
        // 拥有一个 Mutex 对象，执行加锁和解锁操作
        // 当创建这个对象的时候，自动加锁
        // 当这个对象销毁的时候，自动解锁
        public:
            explicit MutexLockGuard(MutexLock& mutex)
              : mutex_(mutex)
            {
                mutex_.lock();
            }

            // 当这个变量销毁时，会自动解锁，防止自己忘记调用 unlock
            // 但是不负责 mutex 锁的销毁，只是解锁
            ~MutexLockGuard()
            {
                mutex_.unlock();
            }
        private:
            MutexLock& mutex_;
    };
}

// Prevent misuse like:
// MutexLockGuard(mutex_);
// A tempory object doesn't hold the lock for long!
#define MutexLockGuard(x) error "Missing guard object name"

#endif  // MUDUO_BASE_MUTEX_H
