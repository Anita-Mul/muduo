#include <muduo/base/Singleton.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/ThreadLocal.h>
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

// 每个线程都有一份 Test 对象，STL 是每个线程中都存在，共有的
#define STL muduo::Singleton<muduo::ThreadLocal<Test> >::instance().value()

void print()
{
    printf("tid=%d, %p name=%s\n",
          muduo::CurrentThread::tid(),
          &STL,
          STL.name().c_str());
}

void threadFunc(const char* changeTo)
{
    print();
    STL.setName(changeTo);
    sleep(1);
    print();
}

int main()
{
    STL.setName("main one");
    muduo::Thread t1(boost::bind(threadFunc, "thread1"));
    muduo::Thread t2(boost::bind(threadFunc, "thread2"));
    t1.start();
    t2.start();
    t1.join();
    print();
    t2.join();
    pthread_exit(0);
}

/*
tid=19651, constructing 0x109c060
tid=19652, constructing 0x7f37380008c0
tid=19652, 0x7f37380008c0 name=
tid=19653, constructing 0x7f37300008c0
tid=19653, 0x7f37300008c0 name=
tid=19652, 0x7f37380008c0 name=thread1
tid=19652, destructing 0x7f37380008c0 thread1
tid=19653, 0x7f37300008c0 name=thread2
tid=19653, destructing 0x7f37300008c0 thread2
tid=19651, 0x109c060 name=main one
tid=19651, destructing 0x109c060 main one
*/