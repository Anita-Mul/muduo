#include "daytime.h"

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <boost/bind.hpp>

using namespace muduo;
using namespace muduo::net;

DaytimeServer::DaytimeServer(EventLoop* loop,
                             const InetAddress& listenAddr)
  : loop_(loop),
    server_(loop, listenAddr, "DaytimeServer")
{
    server_.setConnectionCallback(
        boost::bind(&DaytimeServer::onConnection, this, _1));
    server_.setMessageCallback(
        boost::bind(&DaytimeServer::onMessage, this, _1, _2, _3));
}

void DaytimeServer::start()
{
    server_.start();
}

void DaytimeServer::onConnection(const TcpConnectionPtr& conn)
{
    LOG_INFO << "DaytimeServer - " << conn->peerAddress().toIpPort() << " -> "
            << conn->localAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");

    // Daytime 是短连接协议，在发送完当前时间后，由服务端主动断开连接
    if (conn->connected())
    {
        conn->send(Timestamp::now().toFormattedString() + "\n");
        // 主动断开连接
        conn->shutdown();
    }
}

void DaytimeServer::onMessage(const TcpConnectionPtr& conn,
                              Buffer* buf,
                              Timestamp time)
{
    string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name() << " discards " << msg.size()
            << " bytes received at " << time.toString();
}

