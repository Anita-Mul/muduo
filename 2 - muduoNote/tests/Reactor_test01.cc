#include <muduo/net/EventLoop.h>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

void threadFunc()
{
	printf("threadFunc(): pid = %d, tid = %d\n",
		getpid(), CurrentThread::tid());

	EventLoop loop;
	loop.loop();
}

int main(void)
{
	printf("main(): pid = %d, tid = %d\n",
		getpid(), CurrentThread::tid());

	// 事件循环对象
	EventLoop loop;

	Thread t(threadFunc);
	t.start();

	loop.loop();
	t.join();
	return 0;
}

/*
main(): pid = 31907, tid = 31907
20220316 00:37:44.408018Z 31907 TRACE EventLoop EventLoop created 0x7FFCC054CD20 in thread 31907 - EventLoop.cc:34
20220316 00:37:44.408482Z 31907 TRACE loop EventLoop 0x7FFCC054CD20 start looping - EventLoop.cc:60
threadFunc(): pid = 31907, tid = 31908
20220316 00:37:44.423440Z 31908 TRACE EventLoop EventLoop created 0x7F807A157B90 in thread 31908 - EventLoop.cc:34
20220316 00:37:44.423455Z 31908 TRACE loop EventLoop 0x7F807A157B90 start looping - EventLoop.cc:60
20220316 00:37:49.413443Z 31907 TRACE loop EventLoop 0x7FFCC054CD20 stop looping - EventLoop.cc:64
20220316 00:37:49.435099Z 31908 TRACE loop EventLoop 0x7F807A157B90 stop looping - EventLoop.cc:64
*/
