#include <muduo/net/EventLoop.h>
//#include <muduo/net/EventLoopThread.h>
#include <muduo/base/Thread.h>

#include <boost/bind.hpp>

#include <stdio.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

int cnt = 0;
EventLoop* g_loop;

void printTid()
{
    printf("pid = %d, tid = %d\n", getpid(), CurrentThread::tid());
    printf("now %s\n", Timestamp::now().toString().c_str());
}

void print(const char* msg)
{
    printf("msg %s %s\n", Timestamp::now().toString().c_str(), msg);
    
    if (++cnt == 20)
    {
        g_loop->quit();
    }
}

void cancel(TimerId timer)
{
    g_loop->cancel(timer);
    printf("cancelled at %s\n", Timestamp::now().toString().c_str());
}

int main()
{
    printTid();

    sleep(1);
    {
        EventLoop loop;
        g_loop = &loop;

        print("main");
        loop.runAfter(1, boost::bind(print, "once1"));
        loop.runAfter(1.5, boost::bind(print, "once1.5"));
        loop.runAfter(2.5, boost::bind(print, "once2.5"));
        loop.runAfter(3.5, boost::bind(print, "once3.5"));
        TimerId t45 = loop.runAfter(4.5, boost::bind(print, "once4.5"));
        loop.runAfter(4.2, boost::bind(cancel, t45));
        loop.runAfter(4.8, boost::bind(cancel, t45));
        loop.runEvery(2, boost::bind(print, "every2"));
        TimerId t3 = loop.runEvery(3, boost::bind(print, "every3"));
        loop.runAfter(9.001, boost::bind(cancel, t3));

        loop.loop();
        print("main loop exits");
    }
    /*
    sleep(1);
    {
      EventLoopThread loopThread;
      EventLoop* loop = loopThread.startLoop();
      loop->runAfter(2, printTid);
      sleep(3);
      print("thread loop exits");
    }
    */
}

/*
pid = 48119, tid = 48119
now 1647585104.632561
20220318 06:31:45.635201Z 48119 TRACE updateChannel fd = 4 events = 3 - EPollPoller.cc:104
20220318 06:31:45.635803Z 48119 TRACE EventLoop EventLoop created 0x7FFE9071B6E0 in thread 48119 - EventLoop.cc:44
msg 1647585105.635821 main
20220318 06:31:45.635908Z 48119 TRACE loop EventLoop 0x7FFE9071B6E0 start looping - EventLoop.cc:70
20220318 06:31:46.638171Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:46.643687Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:46.643784Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585106.643751 - TimerQueue.cc:80
msg 1647585106.643820 once1
20220318 06:31:47.136131Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:47.136179Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:47.136192Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585107.136188 - TimerQueue.cc:80
msg 1647585107.136203 once1.5
20220318 06:31:47.638055Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:47.638168Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:47.638199Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585107.638188 - TimerQueue.cc:80
msg 1647585107.638467 every2
20220318 06:31:48.136926Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:48.137043Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:48.137074Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585108.137063 - TimerQueue.cc:80
msg 1647585108.137100 once2.5
20220318 06:31:48.638141Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:48.638257Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:48.638288Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585108.638276 - TimerQueue.cc:80
msg 1647585108.638317 every3
20220318 06:31:49.136926Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:49.137045Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:49.137077Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585109.137065 - TimerQueue.cc:80
msg 1647585109.137102 once3.5
20220318 06:31:49.640354Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:49.640472Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:49.640504Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585109.640493 - TimerQueue.cc:80
msg 1647585109.640531 every2
20220318 06:31:49.838007Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:49.838386Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:49.838451Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585109.838433 - TimerQueue.cc:80
cancelled at 1647585109.838505
20220318 06:31:50.437649Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:50.437769Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:50.437800Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585110.437788 - TimerQueue.cc:80
cancelled at 1647585110.437828
20220318 06:31:51.639929Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:51.640073Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:51.640125Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585111.640109 - TimerQueue.cc:80
msg 1647585111.640163 every3
20220318 06:31:51.641192Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:51.641435Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:51.641463Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585111.641453 - TimerQueue.cc:80
msg 1647585111.641484 every2
20220318 06:31:53.643205Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:53.643325Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:53.643559Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585113.643347 - TimerQueue.cc:80
msg 1647585113.643595 every2
20220318 06:31:54.638554Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:54.639111Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:54.639173Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585114.639159 - TimerQueue.cc:80
cancelled at 1647585114.639205
20220318 06:31:55.645254Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:55.645378Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:55.645411Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585115.645399 - TimerQueue.cc:80
msg 1647585115.645450 every2
20220318 06:31:57.646612Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:57.646733Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:57.646763Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585117.646752 - TimerQueue.cc:80
msg 1647585117.646787 every2
20220318 06:31:59.651041Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:31:59.651117Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:31:59.651138Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585119.651131 - TimerQueue.cc:80
msg 1647585119.651154 every2
20220318 06:32:01.652940Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:32:01.653066Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:32:01.653097Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585121.653086 - TimerQueue.cc:80
msg 1647585121.653122 every2
20220318 06:32:03.654554Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:32:03.654672Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:32:03.654703Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585123.654692 - TimerQueue.cc:80
msg 1647585123.654726 every2
20220318 06:32:05.656134Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:32:05.656253Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:32:05.656284Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585125.656273 - TimerQueue.cc:80
msg 1647585125.656501 every2
20220318 06:32:07.657948Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:32:07.658054Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:32:07.658080Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585127.658071 - TimerQueue.cc:80
msg 1647585127.658101 every2
20220318 06:32:09.664868Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:32:09.665028Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:32:09.665098Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585129.665083 - TimerQueue.cc:80
msg 1647585129.665150 every2
20220318 06:32:11.665293Z 48119 TRACE poll 1 events happended - EPollPoller.cc:65
20220318 06:32:11.665355Z 48119 TRACE printActiveChannels {4: IN }  - EventLoop.cc:166
20220318 06:32:11.665371Z 48119 TRACE readTimerfd TimerQueue::handleRead() 1 at 1647585131.665366 - TimerQueue.cc:80
msg 1647585131.665383 every2
20220318 06:32:11.665398Z 48119 TRACE loop EventLoop 0x7FFE9071B6E0 stop looping - EventLoop.cc:95
msg 1647585131.665569 main loop exits
*/