// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/ThreadPool.h>

#include <muduo/base/Exception.h>

#include <boost/bind.hpp>
#include <assert.h>
#include <stdio.h>

using namespace muduo;

ThreadPool::ThreadPool(const string& name)
  : mutex_(),
    cond_(mutex_),
    name_(name),
    running_(false)
{
}

ThreadPool::~ThreadPool()
{
    if (running_)
    {
        stop();
    }
}

void ThreadPool::start(int numThreads)
{
    assert(threads_.empty());
    running_ = true;
    // 准备 numThreads 的空间
    threads_.reserve(numThreads);

    for (int i = 0; i < numThreads; ++i)
    {
        char id[32];
        snprintf(id, sizeof id, "%d", i);
        threads_.push_back(new muduo::Thread(
              boost::bind(&ThreadPool::runInThread, this), name_+id));
        threads_[i].start();
    }
}

void ThreadPool::stop()
{
    {
        MutexLockGuard lock(mutex_);
        running_ = false;
        // 唤醒所有阻塞在条件变量上的线程
        cond_.notifyAll();
    }

    for_each(threads_.begin(),
            threads_.end(),
            boost::bind(&muduo::Thread::join, _1));
}

// 往线程池中的任务队列添加任务
void ThreadPool::run(const Task& task)
{
    if (threads_.empty())
    {
        // 如果线程池是空的，那么直接执行这个任务
        task();
    }
    else
    {
        MutexLockGuard lock(mutex_);
        queue_.push_back(task);
        cond_.notify();
    }
}

// 获取任务
ThreadPool::Task ThreadPool::take()
{
    MutexLockGuard lock(mutex_);
    // always use a while-loop, due to spurious wakeup
    while (queue_.empty() && running_)
    {
        cond_.wait();
    }

    Task task;

    if(!queue_.empty())
    {
        task = queue_.front();
        queue_.pop_front();
    }

    return task;
}

// 每个线程所执行的函数
void ThreadPool::runInThread()
{
    try
    {
        while (running_)
        {
            // 获取一个任务
            // take() 的返回值取名为 task
            Task task(take());

            if (task)
            {
                // 执行一个任务
                task();
            }
        }
    }
    catch (const Exception& ex)
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
        abort();
    }
    catch (const std::exception& ex)
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }
    catch (...)
    {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw; // rethrow
    }
}

