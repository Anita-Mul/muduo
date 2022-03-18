// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/EventLoop.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>
#include <muduo/net/Poller.h>
#include <muduo/net/TimerQueue.h>

//#include <poll.h>
#include <boost/bind.hpp>

#include <sys/eventfd.h>

using namespace muduo;
using namespace muduo::net;

namespace
{
    // 当前线程EventLoop对象指针
    // 线程局部存储
    __thread EventLoop* t_loopInThisThread = 0;

    const int kPollTimeMs = 10000;

    int createEventfd()
    {
        // eventfd是linux 2.6.22后系统提供的一个轻量级的进程间通信的系统调用，eventfd通过一个进程间共享的64位计数器完成进程间通信
        // https://www.cnblogs.com/kekukele/p/12531824.html
        int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);

        if (evtfd < 0)
        {
            LOG_SYSERR << "Failed in eventfd";
            abort();
        }

        return evtfd;
    }
}

EventLoop* EventLoop::getEventLoopOfCurrentThread()
{
    return t_loopInThisThread;
}

EventLoop::EventLoop()
  : looping_(false),
    quit_(false),
    eventHandling_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    poller_(Poller::newDefaultPoller(this)),
    timerQueue_(new TimerQueue(this)),
    wakeupFd_(createEventfd()),                     // 初始化用于进程间通信的文件描述符
    wakeupChannel_(new Channel(this, wakeupFd_)),   // 创建属于进程间通信文件描述符的通道
    currentActiveChannel_(NULL)
{
    LOG_TRACE << "EventLoop created " << this << " in thread " << threadId_;
    
    // 如果当前线程已经创建了EventLoop对象，终止(LOG_FATAL)
    if (t_loopInThisThread)
    {
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                  << " exists in this thread " << threadId_;
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 设置读的回调函数
    wakeupChannel_->setReadCallback(
        boost::bind(&EventLoop::handleRead, this));
    // we are always reading the wakeupfd
    wakeupChannel_->enableReading();
}


EventLoop::~EventLoop()
{
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}


// 事件循环，该函数不能跨线程调用
// 只能在创建该对象的线程中调用
void EventLoop::loop()
{
    assert(!looping_);
    // 断言当前处于创建该对象的线程中
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    LOG_TRACE << "EventLoop " << this << " start looping";

    //::poll(NULL, 0, 5*1000);
    while (!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        
        //++iteration_;
        if (Logger::logLevel() <= Logger::TRACE)
        {
            printActiveChannels();
        }

        // TODO sort channel by priority
        eventHandling_ = true;
        for (ChannelList::iterator it = activeChannels_.begin();
            it != activeChannels_.end(); ++it)
        {
            currentActiveChannel_ = *it;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = NULL;
        eventHandling_ = false;

        // 去执行别的线程发来的 function 队列
        doPendingFunctors();
    }

    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

// 该函数可以跨线程调用
void EventLoop::quit()
{
    quit_ = true;
    // 如果这个函数不是由当前线程调用的，还需要唤醒当前线程
    if (!isInLoopThread())
    {
        wakeup();
    }
}

// 在I/O线程中执行某个回调函数，该函数可以跨线程调用
void EventLoop::runInLoop(const Functor& cb)
{
    if (isInLoopThread())
    {
        // 如果是当前IO线程调用runInLoop，则同步调用cb
        cb();
    }
    else
    {
        // 如果是其它线程调用runInLoop，则异步地将cb添加到队列
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(const Functor& cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(cb);
    }

    // 调用queueInLoop的线程不是IO线程需要唤醒
    // 或者调用queueInLoop的线程是IO线程，并且此时正在调用pending functor，需要唤醒
    // 只有IO线程的事件回调中调用queueInLoop才不需要唤醒
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
    return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(cb, time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
    return timerQueue_->cancel(timerId);
}

void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();

    if (eventHandling_)
    {
        assert(currentActiveChannel_ == channel ||
            std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }

    poller_->removeChannel(channel);
}


void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadId_ = " << threadId_
              << ", current thread id = " <<  CurrentThread::tid();
}

// 在 quit 中用到
void EventLoop::wakeup()
{
    uint64_t one = 1;
    // ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
    // 就是往 wakeupFd_ 文件描述符中写入数据，那么会唤醒 poll，也就是 while 循环会执行下去
    // quit = false 会退出循环
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    //ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    // 在上面的wakeup函数中，write就是1个字节，如果读到1个字节就知道要退出了
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

/*
    不是简单地在临界区内依次调用Functor，而是把回调列表swap到functors中，
这样一方面减小了临界区的长度（意味着不会阻塞其它线程的queueInLoop()），
另一方面，也避免了死锁（因为Functor可能再次调用queueInLoop()）

    由于doPendingFunctors()调用的Functor可能再次调用queueInLoop(cb)，
这时，queueInLoop()就必须wakeup()，否则新增的cb可能就不能及时调用了

    muduo没有反复执行doPendingFunctors()直到pendingFunctors为空，
这是有意的，否则IO线程可能陷入死循环，无法处理IO事件。
*/
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
      MutexLockGuard lock(mutex_);   // 这个锁保护的范围只有 {} 内
      functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); ++i)
    {
        functors[i]();
    }

    callingPendingFunctors_ = false;
}

void EventLoop::printActiveChannels() const
{
    for (ChannelList::const_iterator it = activeChannels_.begin();
        it != activeChannels_.end(); ++it)
    {
        const Channel* ch = *it;
        LOG_TRACE << "{" << ch->reventsToString() << "} ";
    }
}
