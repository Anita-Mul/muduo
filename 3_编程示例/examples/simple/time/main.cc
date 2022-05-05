#include "time.h"

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;

// 服务端只需要关注“三个半事件”中的“连接已建立”事件
int main()
{
  LOG_INFO << "pid = " << getpid();
  EventLoop loop;
  InetAddress listenAddr(2037);
  TimeServer server(&loop, listenAddr);
  server.start();
  loop.loop();
}

