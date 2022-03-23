#include <muduo/base/ThreadPool.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/CurrentThread.h>

#include <boost/bind.hpp>
#include <stdio.h>

void print()
{
    printf("tid=%d\n", muduo::CurrentThread::tid());
}

void printString(const std::string& str)
{
    printf("tid=%d, str=%s\n", muduo::CurrentThread::tid(), str.c_str());
}

int main()
{
    muduo::ThreadPool pool("MainThreadPool");
    pool.start(5);

    pool.run(print);
    pool.run(print);

    for (int i = 0; i < 100; ++i)
    {
      char buf[32];
      snprintf(buf, sizeof buf, "task %d", i);
      pool.run(boost::bind(printString, std::string(buf)));
    }

    muduo::CountDownLatch latch(1);
    pool.run(boost::bind(&muduo::CountDownLatch::countDown, &latch));
    latch.wait();
    pool.stop();
}

/*
tid=15649
tid=15649
tid=15649, str=task 0
tid=15649, str=task 1
tid=15649, str=task 2
tid=15649, str=task 3
tid=15649, str=task 4
tid=15649, str=task 5
tid=15649, str=task 6
tid=15649, str=task 7
tid=15649, str=task 8
tid=15649, str=task 9
tid=15649, str=task 10
tid=15649, str=task 11
tid=15649, str=task 12
tid=15649, str=task 13
tid=15649, str=task 14
tid=15649, str=task 15
tid=15649, str=task 16
tid=15649, str=task 17
tid=15649, str=task 18
tid=15649, str=task 19
tid=15649, str=task 20
tid=15649, str=task 21
tid=15649, str=task 22
tid=15649, str=task 23
tid=15649, str=task 24
tid=15649, str=task 25
tid=15649, str=task 26
tid=15649, str=task 27
tid=15649, str=task 28
tid=15649, str=task 29
tid=15649, str=task 30
tid=15649, str=task 31
tid=15649, str=task 32
tid=15649, str=task 33
tid=15649, str=task 34
tid=15649, str=task 35
tid=15649, str=task 36
tid=15649, str=task 37
tid=15649, str=task 38
tid=15649, str=task 39
tid=15649, str=task 40
tid=15649, str=task 41
tid=15649, str=task 42
tid=15649, str=task 43
tid=15649, str=task 44
tid=15649, str=task 45
tid=15649, str=task 46
tid=15649, str=task 47
tid=15649, str=task 48
tid=15649, str=task 49
tid=15649, str=task 50
tid=15649, str=task 51
tid=15649, str=task 52
tid=15649, str=task 53
tid=15649, str=task 54
tid=15649, str=task 55
tid=15649, str=task 56
tid=15649, str=task 57
tid=15649, str=task 58
tid=15649, str=task 59
tid=15649, str=task 60
tid=15649, str=task 61
tid=15649, str=task 62
tid=15649, str=task 63
tid=15649, str=task 64
tid=15649, str=task 65
tid=15649, str=task 66
tid=15649, str=task 67
tid=15649, str=task 68
tid=15649, str=task 69
tid=15649, str=task 70
tid=15649, str=task 71
tid=15649, str=task 72
tid=15649, str=task 73
tid=15649, str=task 74
tid=15649, str=task 75
tid=15649, str=task 76
tid=15649, str=task 77
tid=15649, str=task 78
tid=15649, str=task 79
tid=15649, str=task 80
tid=15649, str=task 81
tid=15649, str=task 82
tid=15649, str=task 83
tid=15649, str=task 84
tid=15649, str=task 85
tid=15649, str=task 86
tid=15649, str=task 87
tid=15649, str=task 88
tid=15649, str=task 89
tid=15649, str=task 90
tid=15649, str=task 91
tid=15649, str=task 92
tid=15649, str=task 93
tid=15649, str=task 94
tid=15649, str=task 95
tid=15649, str=task 96
tid=15649, str=task 97
tid=15649, str=task 98
tid=15649, str=task 99

*/