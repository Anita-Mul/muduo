// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#define __STDC_LIMIT_MACROS
#include <muduo/net/TimerQueue.h>

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Timer.h>
#include <muduo/net/TimerId.h>

#include <boost/bind.hpp>

#include <sys/timerfd.h>

namespace muduo
{
    namespace net
    {
        namespace detail
        {
            // 创建定时器
            /*
                int timerfd_create(int clockid, int flags)

                1. clockid：
                CLOCK_REALTIME:系统实时时间,随系统实时时间改变而改变,即从UTC1970-1-1 0:0:0  开始计时,中间时刻
                                如果系统时间被用户改成其他,则对应的时间相应改变
                CLOCK_MONOTONIC:从系统启动这一刻起开始计时,不受系统时间被用户改变的影响
                2. flags：
                TFD_NONBLOCK: 	非阻塞模式)     
                TFD_CLOEXEC:	表示当程序执行exec函数时本fd将被系统自动关闭,表示不传递

                使用时一般  timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
            */
            int createTimerfd()
            {
                int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                              TFD_NONBLOCK | TFD_CLOEXEC);
                if (timerfd < 0)
                {
                    LOG_SYSFATAL << "Failed in timerfd_create";
                }
                return timerfd;
            }

            // 计算超时时刻与当前时间的时间差
            struct timespec howMuchTimeFromNow(Timestamp when)
            {
                int64_t microseconds = when.microSecondsSinceEpoch()
                                      - Timestamp::now().microSecondsSinceEpoch();
                
                if (microseconds < 100)
                {
                    microseconds = 100;
                }

                struct timespec ts;
                // 秒数
                ts.tv_sec = static_cast<time_t>(
                    microseconds / Timestamp::kMicroSecondsPerSecond);
                // 纳秒数
                ts.tv_nsec = static_cast<long>(
                    (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);

                return ts;
            }

            // 清除定时器，避免一直触发
            void readTimerfd(int timerfd, Timestamp now)
            {
                uint64_t howmany;
                // 调用read，清除缓冲区中的数据就可以避免一直触发
                ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
                LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
                if (n != sizeof howmany)
                {
                    LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
                }
            }

            // 重置定时器的超时时间
            void resetTimerfd(int timerfd, Timestamp expiration)
            {
                // wake up loop by timerfd_settime()
                struct itimerspec newValue;
                struct itimerspec oldValue;
                bzero(&newValue, sizeof newValue);
                bzero(&oldValue, sizeof oldValue);

                // 转换超时时刻的类型
                newValue.it_value = howMuchTimeFromNow(expiration);
                int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
                
                if (ret)
                {
                    LOG_SYSERR << "timerfd_settime()";
                }
            }
        }
    }
}



using namespace muduo;
using namespace muduo::net;
using namespace muduo::net::detail;

TimerQueue::TimerQueue(EventLoop* loop)
  : loop_(loop),
    // 在 detail 里，创建一个 timerfd
    // 定时器[timerfd]的超时时间就是 timers_ 
    // 中第一个 timer 的超时时间[timer->expiration()]
    timerfd_(createTimerfd()),  
    timerfdChannel_(loop, timerfd_),
    timers_(),
    callingExpiredTimers_(false)
{
    timerfdChannel_.setReadCallback(
        boost::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    ::close(timerfd_);

    // do not remove channel, since we're in EventLoop::dtor();
    // 因为是使用指针，只要在一个队列中删除就可以了
    for (TimerList::iterator it = timers_.begin();
        it != timers_.end(); ++it)
    {
        delete it->second;
    }
}


// 增加一个定时器
TimerId TimerQueue::addTimer(const TimerCallback& cb,       // 定时器的回调函数
                             Timestamp when,                // 定时器的超时时间
                             double interval)               // 定时器的间隔时间
{
    Timer* timer = new Timer(cb, when, interval);

    // 实现跨线程调用
    loop_->runInLoop(
        boost::bind(&TimerQueue::addTimerInLoop, this, timer));

    // addTimerInLoop(timer);
    return TimerId(timer, timer->sequence());
}

// 本身可以跨线程调用
void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop(
        boost::bind(&TimerQueue::cancelInLoop, this, timerId));
        
    // cancelInLoop(timerId);
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
    loop_->assertInLoopThread();

    // 插入一个定时器，有可能会使得最早到期的定时器发生改变
    bool earliestChanged = insert(timer);

    if (earliestChanged)
    {
        // 重置定时器的超时时刻(timerfd_settime)
        resetTimerfd(timerfd_, timer->expiration());
    }
}

// 取消一个定时器，从 set 中移除就可以
void TimerQueue::cancelInLoop(TimerId timerId)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    
    // 查找该定时器
    ActiveTimerSet::iterator it = activeTimers_.find(timer);
    if (it != activeTimers_.end())
    {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1); (void)n;
        delete it->first; // FIXME: no delete please,如果用了unique_ptr,这里就不需要手动删除了
        activeTimers_.erase(it);
    }
    else if (callingExpiredTimers_)
    {
        // 已经到期，并且正在调用回调函数的定时器
        // 添加到 cancel 队列中，在 reset 中删除
        cancelingTimers_.insert(timer);
    }

    assert(timers_.size() == activeTimers_.size());
}

void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());    // 返回当前时间
    readTimerfd(timerfd_, now);		    // 清除该事件，避免一直触发

    // 获取该时刻之前所有的定时器列表(即超时定时器列表)
    // 只关注最早的定时器，如果最早的定时器超时了，后面有很多跟这个定时器的定时时间一样的
    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;
    cancelingTimers_.clear();

    // safe to callback outside critical section
    for (std::vector<Entry>::iterator it = expired.begin();
        it != expired.end(); ++it)
    {
        // 这里回调定时器处理函数
        it->second->run();
    }
    callingExpiredTimers_ = false;

    // 不是一次性定时器，需要重启
    // 超时定时器列表和当前时间
    // 意思就超时定时器列表本来从那两个set中移除了，但是这些定时器中有的有可能是要重复定时的
    // 要找出这些重复定时的，再添加到那两个队列中
    reset(expired, now);
}

// rvo
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    assert(timers_.size() == activeTimers_.size());
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));

    // 返回第一个未到期的Timer的迭代器
    // lower_bound的含义是返回第一个值>=sentry的元素的iterator
    // 即 *end >= sentry，从而 end->first > now
    TimerList::iterator end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);

    // 将到期的定时器插入到expired中
    std::copy(timers_.begin(), end, back_inserter(expired));

    // 从timers_中移除到期的定时器
    timers_.erase(timers_.begin(), end);

    // 从activeTimers_中移除到期的定时器
    for (std::vector<Entry>::iterator it = expired.begin();
        it != expired.end(); ++it)
    {
        ActiveTimer timer(it->second, it->second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1); (void)n;
    }

    assert(timers_.size() == activeTimers_.size());
    // 不需要拷贝构造，直接返回，rvo优化【release版本】
    return expired;
}

// 超时定时器列表和当前时间
void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
    Timestamp nextExpire;

    for (std::vector<Entry>::const_iterator it = expired.begin();
        it != expired.end(); ++it)
    {
        ActiveTimer timer(it->second, it->second->sequence());

        // 如果是重复的定时器并且是未取消定时器，则重启该定时器
        if (it->second->repeat()
            && cancelingTimers_.find(timer) == cancelingTimers_.end())
        {
            it->second->restart(now);
            insert(it->second);
        }
        else
        {
            // 一次性定时器或者已被取消的定时器是不能重置的，因此删除该定时器
            // FIXME move to a free list
            delete it->second; // FIXME: no delete please
        }
    }

    if (!timers_.empty())
    {
        // 获取最早到期的定时器超时时间
        nextExpire = timers_.begin()->second->expiration();
    }

    if (nextExpire.valid())
    {
        // 重置定时器的超时时刻(timerfd_settime)
        resetTimerfd(timerfd_, nextExpire);
    }
}

bool TimerQueue::insert(Timer* timer)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());

    // 最早到期时间是否改变
    bool earliestChanged = false;
    // 取出到期时间
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();

    // 如果timers_为空或者when小于timers_中的最早到期时间
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }

    {
        // 插入到timers_中  按到期时间排序
        std::pair<TimerList::iterator, bool> result
          = timers_.insert(Entry(when, timer));

        assert(result.second); (void)result;
    }

    {
        // 插入到activeTimers_中  按 timer 序号排序
        std::pair<ActiveTimerSet::iterator, bool> result
          = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second); (void)result;
    }

    assert(timers_.size() == activeTimers_.size());
    // 返回最早到期时间是否有变化
    return earliestChanged;
}



