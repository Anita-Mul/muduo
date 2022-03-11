#include <muduo/base/CountDownLatch.h>
#include <muduo/base/Thread.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <string>
#include <stdio.h>

using namespace muduo;

class Test
{
  public:
      Test(int numThreads)
          : latch_(1),
            threads_(numThreads)
      {
          for (int i = 0; i < numThreads; ++i)
          {
              char name[32];
              snprintf(name, sizeof name, "work thread %d", i);
              threads_.push_back(new muduo::Thread(
                    boost::bind(&Test::threadFunc, this), muduo::string(name)));
          }

          // begin 到 end 执行这个函数
          for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::start, _1));
      }

      void run()
      {
          latch_.countDown();
      }

      void joinAll()
      {
          for_each(threads_.begin(), threads_.end(), boost::bind(&Thread::join, _1));
      }

  private:
      void threadFunc()
      {
          latch_.wait();
          printf("tid=%d, %s started\n",
                CurrentThread::tid(),
                CurrentThread::name());

          

          printf("tid=%d, %s stopped\n",
                CurrentThread::tid(),
                CurrentThread::name());
      }

      CountDownLatch latch_;
      boost::ptr_vector<Thread> threads_;
};

int main()
{
    printf("pid=%d, tid=%d\n", ::getpid(), CurrentThread::tid());
    Test t(3);
    sleep(3);
    printf("pid=%d, tid=%d %s running ...\n", ::getpid(), CurrentThread::tid(), CurrentThread::name());
    t.run();
    t.joinAll();

    printf("number of created threads %d\n", Thread::numCreated());
}


/*
  pid=4241, tid=4241
  pid=4241, tid=4241 main running ...
  tid=4242, work thread 0 started
  tid=4242, work thread 0 stopped
  tid=4243, work thread 1 started
  tid=4243, work thread 1 stopped
  tid=4244, work thread 2 started
  tid=4244, work thread 2 stopped
  number of created threads 3
*/

