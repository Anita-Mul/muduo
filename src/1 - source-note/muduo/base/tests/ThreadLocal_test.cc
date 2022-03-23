#include <muduo/base/ThreadLocal.h>
#include <muduo/base/CurrentThread.h>
#include <muduo/base/Thread.h>

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

// 每个线程都有自己的testObj1
muduo::ThreadLocal<Test> testObj1;
muduo::ThreadLocal<Test> testObj2;

void print()
{
    printf("tid=%d, obj1 %p name=%s\n",
          muduo::CurrentThread::tid(),
    &testObj1.value(),
          testObj1.value().name().c_str());


    printf("tid=%d, obj2 %p name=%s\n",
          muduo::CurrentThread::tid(),
    &testObj2.value(),
          testObj2.value().name().c_str());
}

void threadFunc()
{
    print();
    testObj1.value().setName("changed 1");
    testObj2.value().setName("changed 42");
    print();
}

int main()
{
    testObj1.value().setName("main one");
    print();

    muduo::Thread t1(threadFunc);
    t1.start();
    t1.join();

    testObj2.value().setName("main two");
    print();

    pthread_exit(0);
}

/*
tid=17521, constructing 0x1b0d040
tid=17521, obj1 0x1b0d040 name=main one
tid=17521, constructing 0x1b0d060
tid=17521, obj2 0x1b0d060 name=

tid=17522, constructing 0x7f23dc0008c0
tid=17522, obj1 0x7f23dc0008c0 name=
tid=17522, constructing 0x7f23dc0008e0
tid=17522, obj2 0x7f23dc0008e0 name=
tid=17522, obj1 0x7f23dc0008c0 name=changed 1
tid=17522, obj2 0x7f23dc0008e0 name=changed 42
tid=17522, destructing 0x7f23dc0008c0 changed 1
tid=17522, destructing 0x7f23dc0008e0 changed 42

tid=17521, obj1 0x1b0d040 name=main one
tid=17521, obj2 0x1b0d060 name=main two
tid=17521, destructing 0x1b0d040 main one
tid=17521, destructing 0x1b0d060 main two

*/