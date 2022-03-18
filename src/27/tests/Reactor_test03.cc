#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>

#include <boost/bind.hpp>

#include <stdio.h>
#include <sys/timerfd.h>

using namespace muduo;
using namespace muduo::net;

EventLoop* g_loop;
int timerfd;

void timeout(Timestamp receiveTime)
{
	printf("Timeout!\n");
	uint64_t howmany;
	::read(timerfd, &howmany, sizeof howmany);
	g_loop->quit();
}

int main(void)
{
	EventLoop loop;
	g_loop = &loop;

	// 创建了一个定时器，返回了一个文件描述符
	timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	// 创建一个通道
	Channel channel(&loop, timerfd);
	// 设置读事件的回调函数
	channel.setReadCallback(boost::bind(timeout, _1));
	// 关注这个通道的可读事件
	channel.enableReading();

	// 设置这个定时器的超时时间
	struct itimerspec howlong;
	bzero(&howlong, sizeof howlong);
	howlong.it_value.tv_sec = 1;
	// 这个定时器是一次性的定时器
	::timerfd_settime(timerfd, 0, &howlong, NULL);

	loop.loop();

	::close(timerfd);
}

/*
20220317 14:06:31.637996Z 44440 TRACE EventLoop EventLoop created 0x7FFD338AFB30 in thread 44440 - EventLoop.cc:42
20220317 14:06:31.638097Z 44440 TRACE updateChannel fd = 4 events = 3 - EPollPoller.cc:104
20220317 14:06:31.638111Z 44440 TRACE loop EventLoop 0x7FFD338AFB30 start looping - EventLoop.cc:68
20220317 14:06:32.640431Z 44440 TRACE poll 1 events happended - EPollPoller.cc:65
20220317 14:06:32.640551Z 44440 TRACE printActiveChannels {4: IN }  - EventLoop.cc:139
Timeout!
20220317 14:06:32.640567Z 44440 TRACE loop EventLoop 0x7FFD338AFB30 stop looping - EventLoop.cc:93
*/


