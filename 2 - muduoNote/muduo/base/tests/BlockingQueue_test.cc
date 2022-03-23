#include <muduo/base/BlockingQueue.h>
#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Thread.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <string>
#include <stdio.h>

class Test
{
    public:
        Test(int numThreads)
          : latch_(numThreads),
            threads_(numThreads)
        {
            for (int i = 0; i < numThreads; ++i)
            {
                char name[32];
                snprintf(name, sizeof name, "work thread %d", i);
                threads_.push_back(new muduo::Thread(
                      boost::bind(&Test::threadFunc, this), muduo::string(name)));
            }

            for_each(threads_.begin(), threads_.end(), boost::bind(&muduo::Thread::start, _1));
        }

        void run(int times)
        {
            printf("waiting for count down latch\n");
            latch_.wait();
            printf("all threads started\n");
            // 等待所有线程启动之后，主线程先往里面放东西，其它线程取
            for (int i = 0; i < times; ++i)
            {
                char buf[32];
                snprintf(buf, sizeof buf, "hello %d", i);
                queue_.put(buf);
                printf("tid=%d, put data = %s, size = %zd\n", muduo::CurrentThread::tid(), buf, queue_.size());
            }
        }

        void joinAll()
        {
            for (size_t i = 0; i < threads_.size(); ++i)
            {
                queue_.put("stop");
            }

            for_each(threads_.begin(), threads_.end(), boost::bind(&muduo::Thread::join, _1));
        }

    private:
      void threadFunc()
      {
          printf("tid=%d, %s started\n",
                muduo::CurrentThread::tid(),
                muduo::CurrentThread::name());

          latch_.countDown();
          bool running = true;
          while (running)
          {
            std::string d(queue_.take());
            printf("tid=%d, get data = %s, size = %zd\n", muduo::CurrentThread::tid(), d.c_str(), queue_.size());
            running = (d != "stop");
          }

          printf("tid=%d, %s stopped\n",
                muduo::CurrentThread::tid(),
                muduo::CurrentThread::name());
      }

      muduo::BlockingQueue<std::string> queue_;
      muduo::CountDownLatch latch_;
      boost::ptr_vector<muduo::Thread> threads_;
};

int main()
{
    printf("pid=%d, tid=%d\n", ::getpid(), muduo::CurrentThread::tid());
    Test t(5);
    t.run(100);
    t.joinAll();

    printf("number of created threads %d\n", muduo::Thread::numCreated());
}

/*
pid=6742, tid=6742
waiting for count down latch
tid=6744, work thread 1 started
tid=6745, work thread 2 started
tid=6746, work thread 3 started
tid=6747, work thread 4 started
tid=6743, work thread 0 started
all threads started
tid=6742, put data = hello 0, size = 1
tid=6742, put data = hello 1, size = 2
tid=6742, put data = hello 2, size = 3
tid=6742, put data = hello 3, size = 4
tid=6742, put data = hello 4, size = 5
tid=6742, put data = hello 5, size = 6
tid=6742, put data = hello 6, size = 7
tid=6742, put data = hello 7, size = 8
tid=6742, put data = hello 8, size = 9
tid=6742, put data = hello 9, size = 10
tid=6742, put data = hello 10, size = 11
tid=6742, put data = hello 11, size = 12
tid=6742, put data = hello 12, size = 13
tid=6742, put data = hello 13, size = 14
tid=6742, put data = hello 14, size = 15
tid=6742, put data = hello 15, size = 16
tid=6742, put data = hello 16, size = 17
tid=6742, put data = hello 17, size = 18
tid=6742, put data = hello 18, size = 19
tid=6742, put data = hello 19, size = 20
tid=6742, put data = hello 20, size = 21
tid=6742, put data = hello 21, size = 22
tid=6742, put data = hello 22, size = 23
tid=6742, put data = hello 23, size = 24
tid=6742, put data = hello 24, size = 25
tid=6742, put data = hello 25, size = 26
tid=6742, put data = hello 26, size = 27
tid=6742, put data = hello 27, size = 28
tid=6742, put data = hello 28, size = 29
tid=6742, put data = hello 29, size = 30
tid=6742, put data = hello 30, size = 31
tid=6742, put data = hello 31, size = 32
tid=6742, put data = hello 32, size = 33
tid=6742, put data = hello 33, size = 34
tid=6742, put data = hello 34, size = 35
tid=6742, put data = hello 35, size = 36
tid=6742, put data = hello 36, size = 37
tid=6742, put data = hello 37, size = 38
tid=6742, put data = hello 38, size = 39
tid=6742, put data = hello 39, size = 40
tid=6742, put data = hello 40, size = 41
tid=6742, put data = hello 41, size = 42
tid=6742, put data = hello 42, size = 43
tid=6742, put data = hello 43, size = 44
tid=6742, put data = hello 44, size = 45
tid=6742, put data = hello 45, size = 46
tid=6742, put data = hello 46, size = 47
tid=6742, put data = hello 47, size = 48
tid=6742, put data = hello 48, size = 49
tid=6742, put data = hello 49, size = 50
tid=6742, put data = hello 50, size = 51
tid=6742, put data = hello 51, size = 52
tid=6742, put data = hello 52, size = 53
tid=6742, put data = hello 53, size = 54
tid=6742, put data = hello 54, size = 55
tid=6742, put data = hello 55, size = 56
tid=6742, put data = hello 56, size = 57
tid=6742, put data = hello 57, size = 58
tid=6742, put data = hello 58, size = 59
tid=6742, put data = hello 59, size = 60
tid=6742, put data = hello 60, size = 61
tid=6742, put data = hello 61, size = 62
tid=6742, put data = hello 62, size = 63
tid=6742, put data = hello 63, size = 64
tid=6742, put data = hello 64, size = 65
tid=6742, put data = hello 65, size = 66
tid=6742, put data = hello 66, size = 67
tid=6742, put data = hello 67, size = 68
tid=6742, put data = hello 68, size = 69
tid=6742, put data = hello 69, size = 70
tid=6742, put data = hello 70, size = 71
tid=6742, put data = hello 71, size = 72
tid=6742, put data = hello 72, size = 73
tid=6742, put data = hello 73, size = 74
tid=6742, put data = hello 74, size = 75
tid=6742, put data = hello 75, size = 76
tid=6742, put data = hello 76, size = 77
tid=6742, put data = hello 77, size = 78
tid=6742, put data = hello 78, size = 79
tid=6742, put data = hello 79, size = 80
tid=6742, put data = hello 80, size = 81
tid=6742, put data = hello 81, size = 82
tid=6742, put data = hello 82, size = 83
tid=6742, put data = hello 83, size = 84
tid=6742, put data = hello 84, size = 85
tid=6742, put data = hello 85, size = 86
tid=6742, put data = hello 86, size = 87
tid=6742, put data = hello 87, size = 88
tid=6742, put data = hello 88, size = 89
tid=6742, put data = hello 89, size = 90
tid=6742, put data = hello 90, size = 91
tid=6742, put data = hello 91, size = 92
tid=6742, put data = hello 92, size = 93
tid=6742, put data = hello 93, size = 94
tid=6742, put data = hello 94, size = 95
tid=6742, put data = hello 95, size = 96
tid=6742, put data = hello 96, size = 97
tid=6742, put data = hello 97, size = 98
tid=6742, put data = hello 98, size = 99
tid=6742, put data = hello 99, size = 100
tid=6746, get data = hello 0, size = 104
tid=6746, get data = hello 1, size = 103
tid=6746, get data = hello 2, size = 102
tid=6746, get data = hello 3, size = 101
tid=6746, get data = hello 4, size = 100
tid=6746, get data = hello 5, size = 99
tid=6746, get data = hello 6, size = 98
tid=6746, get data = hello 7, size = 97
tid=6746, get data = hello 8, size = 96
tid=6746, get data = hello 9, size = 95
tid=6746, get data = hello 10, size = 94
tid=6746, get data = hello 11, size = 93
tid=6746, get data = hello 12, size = 92
tid=6746, get data = hello 13, size = 91
tid=6746, get data = hello 14, size = 90
tid=6746, get data = hello 15, size = 89
tid=6746, get data = hello 16, size = 88
tid=6746, get data = hello 17, size = 87
tid=6746, get data = hello 18, size = 86
tid=6746, get data = hello 19, size = 85
tid=6746, get data = hello 20, size = 84
tid=6746, get data = hello 21, size = 83
tid=6746, get data = hello 22, size = 82
tid=6746, get data = hello 23, size = 81
tid=6746, get data = hello 24, size = 80
tid=6746, get data = hello 25, size = 79
tid=6746, get data = hello 26, size = 78
tid=6746, get data = hello 27, size = 77
tid=6746, get data = hello 28, size = 76
tid=6746, get data = hello 29, size = 75
tid=6746, get data = hello 30, size = 74
tid=6746, get data = hello 31, size = 73
tid=6746, get data = hello 32, size = 72
tid=6746, get data = hello 33, size = 71
tid=6746, get data = hello 34, size = 70
tid=6746, get data = hello 35, size = 69
tid=6746, get data = hello 36, size = 68
tid=6746, get data = hello 37, size = 67
tid=6746, get data = hello 38, size = 66
tid=6746, get data = hello 39, size = 65
tid=6746, get data = hello 40, size = 64
tid=6746, get data = hello 41, size = 63
tid=6746, get data = hello 42, size = 62
tid=6746, get data = hello 43, size = 61
tid=6746, get data = hello 44, size = 60
tid=6746, get data = hello 45, size = 59
tid=6746, get data = hello 46, size = 58
tid=6746, get data = hello 47, size = 57
tid=6746, get data = hello 48, size = 56
tid=6746, get data = hello 49, size = 55
tid=6746, get data = hello 50, size = 54
tid=6746, get data = hello 51, size = 53
tid=6746, get data = hello 52, size = 52
tid=6746, get data = hello 53, size = 51
tid=6746, get data = hello 54, size = 50
tid=6746, get data = hello 55, size = 49
tid=6746, get data = hello 56, size = 48
tid=6746, get data = hello 57, size = 47
tid=6746, get data = hello 58, size = 46
tid=6746, get data = hello 59, size = 45
tid=6746, get data = hello 60, size = 44
tid=6746, get data = hello 61, size = 43
tid=6746, get data = hello 62, size = 42
tid=6746, get data = hello 63, size = 41
tid=6746, get data = hello 64, size = 40
tid=6746, get data = hello 65, size = 39
tid=6746, get data = hello 66, size = 38
tid=6746, get data = hello 67, size = 37
tid=6746, get data = hello 68, size = 36
tid=6746, get data = hello 69, size = 35
tid=6746, get data = hello 70, size = 34
tid=6746, get data = hello 71, size = 33
tid=6746, get data = hello 72, size = 32
tid=6746, get data = hello 73, size = 31
tid=6746, get data = hello 74, size = 30
tid=6746, get data = hello 75, size = 29
tid=6746, get data = hello 76, size = 28
tid=6746, get data = hello 77, size = 27
tid=6746, get data = hello 78, size = 26
tid=6746, get data = hello 79, size = 25
tid=6746, get data = hello 80, size = 24
tid=6746, get data = hello 81, size = 23
tid=6746, get data = hello 82, size = 22
tid=6746, get data = hello 83, size = 21
tid=6746, get data = hello 84, size = 20
tid=6746, get data = hello 85, size = 19
tid=6746, get data = hello 86, size = 18
tid=6746, get data = hello 87, size = 17
tid=6746, get data = hello 88, size = 16
tid=6746, get data = hello 89, size = 15
tid=6746, get data = hello 90, size = 14
tid=6746, get data = hello 91, size = 13
tid=6746, get data = hello 92, size = 12
tid=6746, get data = hello 93, size = 11
tid=6746, get data = hello 94, size = 10
tid=6746, get data = hello 95, size = 9
tid=6746, get data = hello 96, size = 8
tid=6746, get data = hello 97, size = 7
tid=6746, get data = hello 98, size = 6
tid=6746, get data = hello 99, size = 5
tid=6746, get data = stop, size = 4
tid=6746, work thread 3 stopped
tid=6747, get data = stop, size = 3
tid=6747, work thread 4 stopped
tid=6745, get data = stop, size = 2
tid=6745, work thread 2 stopped
tid=6744, get data = stop, size = 1
tid=6744, work thread 1 stopped
tid=6743, get data = stop, size = 0
tid=6743, work thread 0 stopped
number of created threads 5

*/