#include "codec.h"

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>

#include <boost/bind.hpp>

#include <set>
#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

class ChatServer : boost::noncopyable
{
    public:
        ChatServer(EventLoop* loop,
                  const InetAddress& listenAddr)
        : loop_(loop),
          server_(loop, listenAddr, "ChatServer"),
          codec_(boost::bind(&ChatServer::onStringMessage, this, _1, _2, _3))
        {
            server_.setConnectionCallback(
                boost::bind(&ChatServer::onConnection, this, _1));
            server_.setMessageCallback(
                boost::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
        }

        void setThreadNum(int numThreads)
        {
            server_.setThreadNum(numThreads);
        }

        void start()
        {
            server_.start();
        }

    private:
        void onConnection(const TcpConnectionPtr& conn)
        {
            LOG_INFO << conn->localAddress().toIpPort() << " -> "
                << conn->peerAddress().toIpPort() << " is "
                << (conn->connected() ? "UP" : "DOWN");

            // 有多个 IO 线程，因而这里的 connections_ 需要用 mutex 保护
            MutexLockGuard lock(mutex_);
            if (conn->connected())
            {
                connections_.insert(conn);
            }
            else
            {
                connections_.erase(conn);
            }
        }

        void onStringMessage(const TcpConnectionPtr&,
                            const string& message,
                            Timestamp)
        {
            // 有多个 IO 线程，因而这里的 connections_ 需要用 mutex 保护
            MutexLockGuard lock(mutex_);
            // 转发消息给所有的客户端
            for (ConnectionList::iterator it = connections_.begin();
                it != connections_.end();
                ++it)
            {
                codec_.send(get_pointer(*it), message);
            }
        }

        typedef std::set<TcpConnectionPtr> ConnectionList;
        EventLoop* loop_;
        TcpServer server_;
        LengthHeaderCodec codec_;
        // 多个线程操作一个 connections_，得给 connections_ 加锁 
        MutexLock mutex_;
        ConnectionList connections_;		// 连接列表
};

/*
    由于 mutex 的存在，多线程并不能并发执行，而是串行的
    因而存在较高的锁竞争，效率比较低

    C1想服务器发送一条消息hello，服务器通过一个 IO 线程转发给所有客户端。
    于此同时 C2 向服务器端发送一条消息 hello2，服务器端通过另一个IO线程
    转发给所有客户端，由于锁的存在，这两个线程并不能并发执行，而是串行的。
    这个时候，客户端数目比较大，第二条消息 hello2 到达各个客户端的延迟也比较大
*/
int main(int argc, char* argv[])
{
    LOG_INFO << "pid = " << getpid();
    if (argc > 1)
    {
        EventLoop loop;
        uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
        InetAddress serverAddr(port);
        ChatServer server(&loop, serverAddr);
        if (argc > 2)
        {
          `server.setThreadNum(atoi(argv[2]));
        }
        server.start();
        loop.loop();
    }
    else
    {
        printf("Usage: %s port [thread_num]\n", argv[0]);
    }
}

