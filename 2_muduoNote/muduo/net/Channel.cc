// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>

#include <sstream>

#include <poll.h>

using namespace muduo;
using namespace muduo::net;

const int Channel::kNoneEvent = 0;
// POLLPRI 有紧迫数据可读
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd__)
  : loop_(loop),
    fd_(fd__),
    events_(0),
    revents_(0),
    index_(-1),
    logHup_(true),
    tied_(false),
    eventHandling_(false)
{
}

Channel::~Channel()
{
    assert(!eventHandling_);
}

void Channel::tie(const boost::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

void Channel::update()
{
    loop_->updateChannel(this);
}

// 调用这个函数之前确保调用disableAll
void Channel::remove()
{
    assert(isNoneEvent());
    loop_->removeChannel(this);
}

// Timestamp主要用于读事件的回调函数
void Channel::handleEvent(Timestamp receiveTime)
{
    boost::shared_ptr<void> guard;

    if (tied_)
    {
        // 提升tie_为shared_ptr，如果提升成功，说明指向一个存在的对象
        guard = tie_.lock();
        if (guard)
        {
            LOG_TRACE << "[6] usecount=" << guard.use_count();
              handleEventWithGuard(receiveTime);
            LOG_TRACE << "[12] usecount=" << guard.use_count();
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

// 查看epoll/或者poll返回的具体是什么事件，并根据事件的类型进行相应的处理
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    eventHandling_ = true;

    /*
    if (revents_ & POLLHUP)
    {
      LOG_TRACE << "1111111111111111";
    }
    if (revents_ & POLLIN)
    {
      LOG_TRACE << "2222222222222222";
    }
    */
    
    // 当事件为挂起并没有可读事件时
    // POLLHUP 对方描述符挂起
    // 当客户端调用 close，服务器端接收到 POLLHUP 和 POLLIN
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        if (logHup_)
        {
            LOG_WARN << "Channel::handle_event() POLLHUP";
        }

        if (closeCallback_) closeCallback_();
    }

    // fd not open (output only)
    // 描述字不是一个打开的文件描述符
    if (revents_ & POLLNVAL)
    {
        LOG_WARN << "Channel::handle_event() POLLNVAL";
    }

    // POLLRDHUP 对等方关闭连接
    // 发生错误或者描述符不可打开
    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (errorCallback_) errorCallback_();
    }

    //关于读的事件
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if (readCallback_) readCallback_(receiveTime);
    }

    // 关于写的事件
    if (revents_ & POLLOUT)
    {
        if (writeCallback_) writeCallback_();
    }

    eventHandling_ = false;
}

// 用来调试的
//把事件编写成一个string
string Channel::reventsToString() const
{
  std::ostringstream oss;
  oss << fd_ << ": ";
  if (revents_ & POLLIN)
    oss << "IN ";
  if (revents_ & POLLPRI)
    oss << "PRI ";
  if (revents_ & POLLOUT)
    oss << "OUT ";
  if (revents_ & POLLHUP)
    oss << "HUP ";
  if (revents_ & POLLRDHUP)
    oss << "RDHUP ";
  if (revents_ & POLLERR)
    oss << "ERR ";
  if (revents_ & POLLNVAL)
    oss << "NVAL ";

  return oss.str().c_str();
}
