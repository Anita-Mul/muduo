#include <muduo/base/ThreadLocalSingleton.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Thread.h>

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <stdio.h>

class Test : boost::noncopyable
{
    public:
        Test()
        {
            printf("tid=%d, constructing %p\n", muduo::CurrentThread::tid(), this);
        }

        ~Test()
        {
            printf("tid=%d, destructing %p %s\n", muduo::CurrentThread::tid(), this, name_.c_str());
        }

        const std::string& name() const { return name_; }
        void setName(const std::string& n) { name_ = n; }

    private:
        std::string name_;
};

void threadFunc(const char* changeTo)
{
    printf("tid=%d, %p name=%s\n",
          muduo::CurrentThread::tid(),
          &muduo::ThreadLocalSingleton<Test>::instance(),
          muduo::ThreadLocalSingleton<Test>::instance().name().c_str());
    muduo::ThreadLocalSingleton<Test>::instance().setName(changeTo);
    printf("tid=%d, %p name=%s\n",
          muduo::CurrentThread::tid(),
          &muduo::ThreadLocalSingleton<Test>::instance(),
          muduo::ThreadLocalSingleton<Test>::instance().name().c_str());

    // no need to manually delete it
    // muduo::ThreadLocalSingleton<Test>::destroy();
}

int main()
{
    // 每个线程都有一份
    muduo::ThreadLocalSingleton<Test>::instance().setName("main one");
    muduo::Thread t1(boost::bind(threadFunc, "thread1"));
    muduo::Thread t2(boost::bind(threadFunc, "thread2"));
    t1.start();
    t2.start();
    t1.join();
    printf("tid=%d, %p name=%s\n",
          muduo::CurrentThread::tid(),
          &muduo::ThreadLocalSingleton<Test>::instance(),
          muduo::ThreadLocalSingleton<Test>::instance().name().c_str());
    t2.join();

    pthread_exit(0);
}

/*
tid=19674, constructing 0x16fb040
tid=19676, constructing 0x7f96a80008c0
tid=19676, 0x7f96a80008c0 name=
tid=19676, 0x7f96a80008c0 name=thread2
tid=19676, destructing 0x7f96a80008c0 thread2
tid=19675, constructing 0x7f96a80008c0
tid=19675, 0x7f96a80008c0 name=
tid=19675, 0x7f96a80008c0 name=thread1
tid=19675, destructing 0x7f96a80008c0 thread1
tid=19674, 0x16fb040 name=main one
tid=19674, destructing 0x16fb040 main one
*/
