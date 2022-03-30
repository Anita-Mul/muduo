#include "echo.h"

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

// using namespace muduo;
// using namespace muduo::net;

// TCP 服务器
int main()
{
    LOG_INFO << "pid = " << getpid();

    muduo::net::EventLoop loop;
    muduo::net::InetAddress listenAddr(2007);

    EchoServer server(&loop, listenAddr);

    server.start();
    loop.loop();
}

