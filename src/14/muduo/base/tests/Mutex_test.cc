//#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Timestamp.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <vector>
#include <stdio.h>

using namespace muduo;
using namespace std;

MutexLock g_mutex;
vector<int> g_vec;
const int kCount = 10*1000*1000;

void threadFunc()
{
    for (int i = 0; i < kCount; ++i)
    {
        MutexLockGuard lock(g_mutex);
        g_vec.push_back(i);
    }
}

int main()
{
    const int kMaxThreads = 8;
    // 预留 80000000 内存
    g_vec.reserve(kMaxThreads * kCount);

    Timestamp start(Timestamp::now());
    for (int i = 0; i < kCount; ++i)
    {
        g_vec.push_back(i);
    }

    printf("single thread without lock %f\n", timeDifference(Timestamp::now(), start));

    start = Timestamp::now();
    threadFunc();
    printf("single thread with lock %f\n", timeDifference(Timestamp::now(), start));

    for (int nthreads = 1; nthreads < kMaxThreads; ++nthreads)
    {
        // 里面存放的是 thread 的指针
        boost::ptr_vector<Thread> threads;
        g_vec.clear();
        
        start = Timestamp::now();
        for (int i = 0; i < nthreads; ++i)
        {
            threads.push_back(new Thread(&threadFunc));
            threads.back().start();
        }
        for (int i = 0; i < nthreads; ++i)
        {
            threads[i].join();
        }
        printf("%d thread(s) with lock %f\n", nthreads, timeDifference(Timestamp::now(), start));
    }
}

/*
    single thread without lock 0.369520
    single thread with lock 1.658937
    1 thread(s) with lock 1.102071
    2 thread(s) with lock 2.361683
    3 thread(s) with lock 4.595901
    4 thread(s) with lock 5.165572
    5 thread(s) with lock 7.465654
    6 thread(s) with lock 9.076668
    7 thread(s) with lock 9.143767
*/
