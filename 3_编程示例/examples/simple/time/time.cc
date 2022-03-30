#include "time.h"

#include <muduo/base/Logging.h>
#include <muduo/net/Endian.h>

#include <boost/bind.hpp>

using namespace muduo;
using namespace muduo::net;

TimeServer::TimeServer(muduo::net::EventLoop* loop,
                             const muduo::net::InetAddress& listenAddr)
  : loop_(loop),
    server_(loop, listenAddr, "TimeServer")
{
    server_.setConnectionCallback(
        boost::bind(&TimeServer::onConnection, this, _1));
    server_.setMessageCallback(
        boost::bind(&TimeServer::onMessage, this, _1, _2, _3));
}

void TimeServer::start()
{
    server_.start();
}

void TimeServer::onConnection(const muduo::net::TcpConnectionPtr& conn)
{
    LOG_INFO << "TimeServer - " << conn->peerAddress().toIpPort() << " -> "
            << conn->localAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");

    if (conn -> connected())
    {
        // Time 协议与 daytime 极为类似，只不过它返回的不是日期时间字符串，而是
        // 一个 32-bit 整数，表示从 1970-01-01 00:00:00Z 到现在的秒数
        time_t now = ::time(NULL);
        int32_t be32 = sockets::hostToNetwork32(static_cast<int32_t>(now));
        
        conn->send(&be32, sizeof be32);
        conn->shutdown();
    }
}

void TimeServer::onMessage(const muduo::net::TcpConnectionPtr& conn,
                 muduo::net::Buffer* buf,
                 muduo::Timestamp time)
{
    string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name() << " discards " << msg.size()
            << " bytes received at " << time.toString();
}

