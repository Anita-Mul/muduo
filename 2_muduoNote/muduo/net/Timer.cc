// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/Timer.h>

using namespace muduo;
using namespace muduo::net;

AtomicInt64 Timer::s_numCreated_;

void Timer::restart(Timestamp now)
{
    // 如果是重复的定时器
    if (repeat_)
    {
        // 重新计算下一个超时时刻
        expiration_ = addTime(now, interval_);
    }
    else
    {
        // 下一次的超时时刻等于一个非法的时间
        expiration_ = Timestamp::invalid();
    }
}



