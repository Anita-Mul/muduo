#include "codec.h"

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/ThreadLocalSingleton.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

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
            server_.setThreadInitCallback(boost::bind(&ChatServer::threadInit, this, _1));
            server_.start();
        }

    private:
        void onConnection(const TcpConnectionPtr& conn)
        {
            LOG_INFO << conn->localAddress().toIpPort() << " -> "
                    << conn->peerAddress().toIpPort() << " is "
                    << (conn->connected() ? "UP" : "DOWN");

            if (conn->connected())
            {
                // 不需要用锁，每个线程都有一个 connections_
                connections_.instance().insert(conn);
            }
            else
            {
                connections_.instance().erase(conn);
            }
        }

        void onStringMessage(const TcpConnectionPtr&,
                            const string& message,
                            Timestamp)
        {
            EventLoop::Functor f = boost::bind(&ChatServer::distributeMessage, this, message);
            LOG_DEBUG;

            MutexLockGuard lock(mutex_);

            // 转发消息给所有客户端，高效转发（多线程来转发）
            for (std::set<EventLoop*>::iterator it = loops_.begin();
                it != loops_.end();
                ++it)
            {
                // 1. 让对应的 IO 线程来执行 distributeMessage
                // 2. distributeMessage 放到 IO 线程队列中执行，因此，这里的 mutex_ 锁竞争大大减小
                // 3. distributeMessage 不受 mutex_ 保护
                (*it)->queueInLoop(f);
            }
            
            LOG_DEBUG;
        }

        typedef std::set<TcpConnectionPtr> ConnectionList;

        void distributeMessage(const string& message)
        {
            LOG_DEBUG << "begin";

            for (ConnectionList::iterator it = connections_.instance().begin();
                it != connections_.instance().end();
                ++it)
            {
                codec_.send(get_pointer(*it), message);
            }
            
            LOG_DEBUG << "end";
        }

        void threadInit(EventLoop* loop)
        {
            assert(connections_.pointer() == NULL);
            connections_.instance();
            assert(connections_.pointer() != NULL);
            MutexLockGuard lock(mutex_);
            loops_.insert(loop);
        }

        EventLoop* loop_;
        TcpServer server_;
        LengthHeaderCodec codec_;
        // 线程局部单例变量，每个线程都有一个 connections 实例
        ThreadLocalSingleton<ConnectionList> connections_;

        MutexLock mutex_;
        std::set<EventLoop*> loops_;        // EventLoop 列表
};

/*
    之前是通过一个线程来转发消息给所有的客户端

    现在
        T1 线程转发消息给 C1，T2 线程转发消息给 C2
        T3 线程转发消息给 C3，T4 线程转发消息给 C4
        T1 线程转发消息给 C5，T2 线程转发消息给 C6
        T3 线程转发消息给 C7，T2 线程转发消息给 C8
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
            server.setThreadNum(atoi(argv[2]));
        }

        server.start();
        loop.loop();
    }
    else
    {
        printf("Usage: %s port [thread_num]\n", argv[0]);
    }
}


