#ifndef MUDUO_EXAMPLES_SIMPLE_ECHO_ECHO_H
#define MUDUO_EXAMPLES_SIMPLE_ECHO_ECHO_H

#include <muduo/net/TcpServer.h>

// RFC 862
// 实现一个简单的回射服务器
/*
    网络编程关注三个半事件：
    连接建立
    连接断开
    消息到达
    消息发送完毕（对于低流量的服务来说，通常不需要关注该事件）
*/
class EchoServer
{
    public:
        EchoServer(muduo::net::EventLoop* loop,
                  const muduo::net::InetAddress& listenAddr);

        void start();  // calls server_.start();

    private:
        // TcpServer 所对应的 onConnection 和 onMessage
        void onConnection(const muduo::net::TcpConnectionPtr& conn);

        void onMessage(const muduo::net::TcpConnectionPtr& conn,
                      muduo::net::Buffer* buf,
                      muduo::Timestamp time);

        muduo::net::TcpServer server_;
        muduo::net::EventLoop* loop_;
};

#endif  // MUDUO_EXAMPLES_SIMPLE_ECHO_ECHO_H
