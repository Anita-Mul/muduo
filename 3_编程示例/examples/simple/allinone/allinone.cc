#include "../chargen/chargen.h"
#include "../daytime/daytime.h"
#include "../discard/discard.h"
#include "../echo/echo.h"
#include "../time/time.h"

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;


/*
前面五个程序都用到了 EventLoop，这其实是个 Reactor，用于注册和分发 IO
事件。Muduo 遵循 one loop per thread 模型，多个服务端 (TcpServer) 和客户端
(TcpClient) 可以共享同一个 EventLoop，也可以分配到多个 EventLoop 上以发挥多
核多线程的好处。这里我们把五个服务端用同一个 EventLoop 跑起来，程序还是单
线程的，功能却强大了很多
*/
int main()
{
    LOG_INFO << "pid = " << getpid();
    EventLoop loop;  // one loop shared by multiple servers

    ChargenServer chargenServer(&loop, InetAddress(2019));
    chargenServer.start();

    DaytimeServer daytimeServer(&loop, InetAddress(2013));
    daytimeServer.start();

    DiscardServer discardServer(&loop, InetAddress(2009));
    discardServer.start();

    EchoServer echoServer(&loop, InetAddress(2007));
    echoServer.start();

    TimeServer timeServer(&loop, InetAddress(2037));
    timeServer.start();

    loop.loop();
}

