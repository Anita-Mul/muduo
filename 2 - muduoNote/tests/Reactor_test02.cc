#include <muduo/net/EventLoop.h>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

EventLoop* g_loop;

void threadFunc()
{
	g_loop->loop();
}

int main(void)
{
	EventLoop loop;
	g_loop = &loop;
	Thread t(threadFunc);
	t.start();
	t.join();
	return 0;
}

/*
20220316 00:38:38.524729Z 31924 TRACE EventLoop EventLoop created 0x7FFCEF86A800 in thread 31924 - EventLoop.cc:34
20220316 00:38:38.547784Z 31925 FATAL EventLoop::abortNotInLoopThread - EventLoop 0x7FFCEF86A800 was created in threadId_ = 31924, current thread id = 31925 - EventLoop.cc:70
已放弃(吐核)
*/