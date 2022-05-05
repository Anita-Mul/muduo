#include "chargen.h"

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <boost/bind.hpp>
#include <stdio.h>

/**
 * Chargen协议很特殊，它只发送数据，不接收数据。而且，它发送数据的速度
 * 不能快过客户端接收的速度，因此需要关注“三个半事件”中的半个“消息／数
 * 据发送完毕”事件（onWriteComplete）
 */
using namespace muduo;
using namespace muduo::net;

ChargenServer::ChargenServer(EventLoop* loop,
                             const InetAddress& listenAddr,
                             bool print)
  : loop_(loop),
    server_(loop, listenAddr, "ChargenServer"),
    transferred_(0),
    startTime_(Timestamp::now())
{
    server_.setConnectionCallback(
        boost::bind(&ChargenServer::onConnection, this, _1));
    server_.setMessageCallback(
        boost::bind(&ChargenServer::onMessage, this, _1, _2, _3));
    server_.setWriteCompleteCallback(
        boost::bind(&ChargenServer::onWriteComplete, this, _1));
    
    if (print)
    {
        // 每 3s 执行一次，打印吞吐量
        loop->runEvery(3.0, boost::bind(&ChargenServer::printThroughput, this));
    }

    // 初始化 line
    /*
        ASCII 33-126 为可打印字符
        33 34 35 ... 104
        34 35 36 ... 105
        ...
        55 56 57 ... 126
    */
    string line;
    for (int i = 33; i < 127; ++i)
    {
        line.push_back(char(i));
    }
    line += line;

    // 初始化 mesage_ 【大流量数据】
    for (size_t i = 0; i < 127-33; ++i)
    {
        message_ += line.substr(i, 72) + '\n';
    }
}


void ChargenServer::start()
{
  server_.start();
}


void ChargenServer::onConnection(const TcpConnectionPtr& conn)
{
  LOG_INFO << "ChargenServer - " << conn->peerAddress().toIpPort() << " -> "
           << conn->localAddress().toIpPort() << " is "
           << (conn->connected() ? "UP" : "DOWN");
  
  if (conn->connected())
  {
      // 设置端口复用
      conn->setTcpNoDelay(true);
      // 发送 message_ 
      conn->send(message_);
  }
}


void ChargenServer::onMessage(const TcpConnectionPtr& conn,
                              Buffer* buf,
                              Timestamp time)
{
    string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name() << " discards " << msg.size()
            << " bytes received at " << time.toString();
}


// 发送大流量数据需要 onWriteComplete 回调函数
// onWriteComplete 回调说明发送缓冲区的数据全部发送完毕
void ChargenServer::onWriteComplete(const TcpConnectionPtr& conn)
{
    // transferred_ 是总发送数据的大小
    transferred_ += message_.size();
    // 再次发送数据
    conn->send(message_);
}


void ChargenServer::printThroughput()
{
    Timestamp endTime = Timestamp::now();
    double time = timeDifference(endTime, startTime_);
    printf("%4.3f MiB/s\n", static_cast<double>(transferred_)/time/1024/1024);
    transferred_ = 0;
    startTime_ = endTime;
}

