// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/EventLoopThread.h>

#include <muduo/net/EventLoop.h>

#include <boost/bind.hpp>

using namespace muduo;
using namespace muduo::net;


EventLoopThread::EventLoopThread(const ThreadInitCallback& cb)
  : loop_(NULL),
    exiting_(false),
    thread_(boost::bind(&EventLoopThread::threadFunc, this)),
    mutex_(),
    cond_(mutex_),
    callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    loop_->quit();		// 退出IO线程，让IO线程的loop循环退出，从而退出了IO线程
    thread_.join();   // 退出线程
}

EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
    thread_.start();

    {
        MutexLockGuard lock(mutex_);
        // loop_ 指针指向一个 eventLoop 对象
        // loop_ 不为空，说明新创建的线程运行起来了
        while (loop_ == NULL)
        {
            cond_.wait();
        }
    }

    return loop_;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;

    // 在构造函数中可以选择传递 callback_，也可以选择不传递 callback_
    if (callback_)
    {
        callback_(&loop);
    }

    {
        MutexLockGuard lock(mutex_);
        // loop_指针指向了一个栈上的对象，threadFunc函数退出之后，这个指针就失效了
        // threadFunc函数退出，就意味着线程退出了，EventLoopThread对象也就没有存在的价值了。
        // 因而不会有什么大的问题
        loop_ = &loop;
        cond_.notify();
    }

    loop.loop();
    //assert(exiting_);
}

