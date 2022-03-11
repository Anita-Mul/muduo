// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/Condition.h>

#include <errno.h>

// returns true if time out, false otherwise.
bool muduo::Condition::waitForSeconds(int seconds)
{
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += seconds;
    // 当在指定时间内有信号传过来时，pthread_cond_timedwait()返回0，否则返回一个非0数
    return ETIMEDOUT == pthread_cond_timedwait(&pcond_, mutex_.getPthreadMutex(), &abstime);
}

