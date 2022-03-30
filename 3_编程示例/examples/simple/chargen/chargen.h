#ifndef MUDUO_EXAMPLES_SIMPLE_CHARGEN_CHARGEN_H
#define MUDUO_EXAMPLES_SIMPLE_CHARGEN_CHARGEN_H

#include <muduo/net/TcpServer.h>

// 在 TCP 连接建立后，服务器不断传送任意的字符到客户端，直到客户端关闭连接
// TCP 流量调节的功能：即使服务端生成数据很快，但是客户端接收或处理数据慢
// RFC 864
class ChargenServer
{
    public:
        ChargenServer(muduo::net::EventLoop* loop,
                        const muduo::net::InetAddress& listenAddr,
                        bool print = false);

        void start();

    private:
        void onConnection(const muduo::net::TcpConnectionPtr& conn);

        void onMessage(const muduo::net::TcpConnectionPtr& conn,
                        muduo::net::Buffer* buf,
                        muduo::Timestamp time);

        void onWriteComplete(const muduo::net::TcpConnectionPtr& conn);
        void printThroughput();

        muduo::net::EventLoop* loop_;
        muduo::net::TcpServer server_;

        muduo::string message_;
        int64_t transferred_;
        muduo::Timestamp startTime_;
};

#endif  // MUDUO_EXAMPLES_SIMPLE_CHARGEN_CHARGEN_H
