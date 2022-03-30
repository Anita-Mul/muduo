#include <muduo/base/BlockingQueue.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Timestamp.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <map>
#include <string>
#include <stdio.h>

class Bench
{
    public:
        Bench(int numThreads)
            : latch_(numThreads),
            threads_(numThreads)
        {
            for (int i = 0; i < numThreads; ++i)
            {
                char name[32];
                snprintf(name, sizeof name, "work thread %d", i);
                threads_.push_back(new muduo::Thread(
                    boost::bind(&Bench::threadFunc, this), muduo::string(name)));
            }

            for_each(threads_.begin(), threads_.end(), boost::bind(&muduo::Thread::start, _1));
        }

        // 等待所有线程都准备好之后，执行这个
        void run(int times)
        {
            printf("waiting for count down latch\n");
            latch_.wait();
            printf("all threads started\n");
            for (int i = 0; i < times; ++i)
            {
                muduo::Timestamp now(muduo::Timestamp::now());
                queue_.put(now);
                usleep(1000);
            }
        }

        void joinAll()
        {
            for (size_t i = 0; i < threads_.size(); ++i)
            {
                queue_.put(muduo::Timestamp::invalid());
            }

            for_each(threads_.begin(), threads_.end(), boost::bind(&muduo::Thread::join, _1));
        }

    private:
        // 线程调用的函数
        void threadFunc()
        {
            printf("tid=%d, %s started\n",
                    muduo::CurrentThread::tid(),
                    muduo::CurrentThread::name());

            std::map<int, int> delays;
            latch_.countDown();
            bool running = true;
            while (running)
            {
                muduo::Timestamp t(queue_.take());
                muduo::Timestamp now(muduo::Timestamp::now());
                if (t.valid())
                {
                    int delay = static_cast<int>(timeDifference(now, t) * 1000000);
                    // printf("tid=%d, latency = %d us\n",
                    //        muduo::CurrentThread::tid(), delay);
                    ++delays[delay];
                }

                // 当 t.valid 无效的时候，就是 join 的时候 
                running = t.valid();
            }

            printf("tid=%d, %s stopped\n",
                    muduo::CurrentThread::tid(),
                    muduo::CurrentThread::name());
            for (std::map<int, int>::iterator it = delays.begin();
                it != delays.end(); ++it)
            {
                printf("tid = %d, delay = %d, count = %d\n",
                        muduo::CurrentThread::tid(),
                        it->first, it->second);
            }
        }

        muduo::BlockingQueue<muduo::Timestamp> queue_;
        // 某一线程在开始运行前等待n个线程执行完毕。将CountDownLatch的计数器初始化为new CountDownLatch(n)
        // 每当一个任务线程执行完毕，就将计数器减1 countdownLatch.countDown()，当计数器的值变为0时，
        // 在CountDownLatch上await()的线程就会被唤醒。一个典型应用场景就是启动一个服务时，主线程需要等待多个组件加载完毕，之后再继续执行
        muduo::CountDownLatch latch_;
        boost::ptr_vector<muduo::Thread> threads_;
};

int main(int argc, char* argv[])
{
    int threads = argc > 1 ? atoi(argv[1]) : 1;

    Bench t(threads);
    t.run(10000);
    t.joinAll();
}

/*
waiting for count down latch
tid=6725, work thread 0 started
all threads started
tid=6725, work thread 0 stopped
tid = 6725, delay = 5, count = 9
tid = 6725, delay = 6, count = 162
tid = 6725, delay = 7, count = 165
tid = 6725, delay = 8, count = 130
tid = 6725, delay = 9, count = 132
tid = 6725, delay = 10, count = 158
tid = 6725, delay = 11, count = 208
tid = 6725, delay = 12, count = 310
tid = 6725, delay = 13, count = 585
tid = 6725, delay = 14, count = 454
tid = 6725, delay = 15, count = 487
tid = 6725, delay = 16, count = 402
tid = 6725, delay = 17, count = 393
tid = 6725, delay = 18, count = 442
tid = 6725, delay = 19, count = 531
tid = 6725, delay = 20, count = 379
tid = 6725, delay = 21, count = 996
tid = 6725, delay = 22, count = 1653
tid = 6725, delay = 23, count = 421
tid = 6725, delay = 24, count = 233
tid = 6725, delay = 25, count = 369
tid = 6725, delay = 26, count = 279
tid = 6725, delay = 27, count = 140
tid = 6725, delay = 28, count = 113
tid = 6725, delay = 29, count = 132
tid = 6725, delay = 30, count = 141
tid = 6725, delay = 31, count = 73
tid = 6725, delay = 32, count = 47
tid = 6725, delay = 33, count = 28
tid = 6725, delay = 34, count = 40
tid = 6725, delay = 35, count = 79
tid = 6725, delay = 36, count = 71
tid = 6725, delay = 37, count = 29
tid = 6725, delay = 38, count = 20
tid = 6725, delay = 39, count = 21
tid = 6725, delay = 40, count = 28
tid = 6725, delay = 41, count = 21
tid = 6725, delay = 42, count = 16
tid = 6725, delay = 43, count = 8
tid = 6725, delay = 44, count = 7
tid = 6725, delay = 45, count = 6
tid = 6725, delay = 46, count = 4
tid = 6725, delay = 47, count = 3
tid = 6725, delay = 48, count = 4
tid = 6725, delay = 49, count = 4
tid = 6725, delay = 50, count = 2
tid = 6725, delay = 51, count = 2
tid = 6725, delay = 52, count = 3
tid = 6725, delay = 53, count = 3
tid = 6725, delay = 54, count = 1
tid = 6725, delay = 55, count = 2
tid = 6725, delay = 58, count = 1
tid = 6725, delay = 59, count = 1
tid = 6725, delay = 61, count = 2
tid = 6725, delay = 64, count = 3
tid = 6725, delay = 65, count = 1
tid = 6725, delay = 67, count = 1
tid = 6725, delay = 77, count = 2
tid = 6725, delay = 80, count = 1
tid = 6725, delay = 99, count = 1
tid = 6725, delay = 103, count = 1
tid = 6725, delay = 106, count = 2
tid = 6725, delay = 107, count = 1
tid = 6725, delay = 112, count = 1
tid = 6725, delay = 113, count = 1
tid = 6725, delay = 120, count = 1
tid = 6725, delay = 124, count = 1
tid = 6725, delay = 132, count = 1
tid = 6725, delay = 136, count = 1
tid = 6725, delay = 142, count = 2
tid = 6725, delay = 143, count = 1
tid = 6725, delay = 144, count = 1
tid = 6725, delay = 149, count = 1
tid = 6725, delay = 150, count = 1
tid = 6725, delay = 153, count = 1
tid = 6725, delay = 157, count = 1
tid = 6725, delay = 179, count = 1
tid = 6725, delay = 209, count = 1
tid = 6725, delay = 222, count = 1
tid = 6725, delay = 227, count = 1
tid = 6725, delay = 241, count = 1
tid = 6725, delay = 255, count = 1
tid = 6725, delay = 278, count = 1
tid = 6725, delay = 292, count = 1
tid = 6725, delay = 300, count = 1
tid = 6725, delay = 323, count = 1
tid = 6725, delay = 343, count = 1
tid = 6725, delay = 348, count = 1
tid = 6725, delay = 520, count = 1
tid = 6725, delay = 531, count = 1
tid = 6725, delay = 532, count = 1
tid = 6725, delay = 563, count = 1
tid = 6725, delay = 740, count = 1
tid = 6725, delay = 888, count = 1
tid = 6725, delay = 936, count = 1
tid = 6725, delay = 1095, count = 1
tid = 6725, delay = 1955, count = 1
tid = 6725, delay = 5767, count = 1
tid = 6725, delay = 9589, count = 1

*/