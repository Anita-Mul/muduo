// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>

#include <boost/noncopyable.hpp>

namespace muduo
{
    /**
     *  - 既可以用于所有子线程等待主线程发起 “起跑” 
     *  - 也可以用于主线程等待子线程初始化完毕才开始工作
     */
    class CountDownLatch : boost::noncopyable
    {
        public:
            explicit CountDownLatch(int count);
            void wait();
            void countDown();
            int getCount() const;

        private:
            // 可以改变 mutable 修饰成员的状态
            mutable MutexLock mutex_;     // 互斥锁
            Condition condition_;         // 条件变量
            int count_;
    };
}
#endif  // MUDUO_BASE_COUNTDOWNLATCH_H
