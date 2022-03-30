#include <muduo/base/Exception.h>
#include <stdio.h>

class Bar
{
    public:
        void test()
        {
          throw muduo::Exception("oops");
        }
};

void foo()
{
    Bar b;
    b.test();
}

int main()
{
    try
    {
        foo();
    }
    catch (const muduo::Exception& ex)
    {
        printf("reason: %s\n", ex.what());
        printf("stack trace: %s\n", ex.stackTrace());
    }
}

/*
reason: oops
stack trace: muduo::Exception::fillStackTrace()
muduo::Exception::Exception(char const*)
Bar::test()
foo()
./exception_test(main+0xe)
/lib64/libc.so.6(__libc_start_main+0xf5)
./exception_test()

*/
