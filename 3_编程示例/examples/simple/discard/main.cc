#include "discard.h"

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;

/*
  丢弃所有收到的数据
  discard恐怕算是最简单的长连接TCP应用层协议，它只需要关注“三个半事件”中的“消息／数据到达”事件
*/
int main()
{
    LOG_INFO << "pid = " << getpid();

    EventLoop loop;
    InetAddress listenAddr(2009);

    DiscardServer server(&loop, listenAddr);
    
    server.start();
    loop.loop();
}

