## EventLoopThread
 - 任何一个线程，只要创建并运行了EventLoop，都称之为IO线程
 - IO线程不一定是主线程
 - muduo并发模型one loop per thread + threadpool
 - 为了方便今后使用，定义了EventLoopThread类，该类封装了IO线程
    > EventLoopThread创建了一个线程
    > 在线程函数中创建了一个EvenLoop对象并调用EventLoop::loop
