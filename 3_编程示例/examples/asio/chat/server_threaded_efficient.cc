#include "codec.h"

#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
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
          codec_(boost::bind(&ChatServer::onStringMessage, this, _1, _2, _3)),
          connections_(new ConnectionList)   // connections_ 的引用计数就是1
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

            // 加锁
            MutexLockGuard lock(mutex_);

            if (!connections_.unique())		        // 说明引用计数大于 2
            {
                // new ConnectionList(*connections_) 这段代码拷贝了一份 ConnectionList
                // 原来的 connecions 减1，如果读的全读完了， connecions 为 0，自动删除
                connections_.reset(new ConnectionList(*connections_));
            }
            assert(connections_.unique());
            
            // 在副本上修改，不会影响，所以杜泽在遍历列表的时候，不需要用 mutex 保护
            if (conn->connected())
            {
                connections_->insert(conn);
            }
            else
            {
                connections_->erase(conn);
            }
        }

        typedef std::set<TcpConnectionPtr> ConnectionList;
        // 采用引用计数的智能指针，就是 ConnectionList 的智能指针，写的时候在副本上写，读的时候引用加1
        typedef boost::shared_ptr<ConnectionList> ConnectionListPtr;

        void onStringMessage(const TcpConnectionPtr&,
                            const string& message,
                            Timestamp)
        {
            // 引用计数加1，mutex保护的临界区大大缩短
            ConnectionListPtr connections = getConnectionList();

            // 可能大家会有疑问，不受 mutex 保护，写者更改了连接列表怎么办
            // 实际上，写者是在另一个副本上修改，所以无需担心
            for (ConnectionList::iterator it = connections->begin();
                it != connections->end();
                ++it)
            {
              codec_.send(get_pointer(*it), message);
            }
            // 这个断言不一定成立
            // assert(!connections.unique());
            // 当 connections 这个栈上的变量销毁的时候，引用计数减1
        }

        ConnectionListPtr getConnectionList()
        {
            MutexLockGuard lock(mutex_);
            return connections_;
        }

        EventLoop* loop_;
        TcpServer server_;
        LengthHeaderCodec codec_;
        // 多个线程操作一个 connections_，得给 connections_ 加锁 
        MutexLock mutex_;
        ConnectionListPtr connections_;    // 对这个列表有写和读的操作
};

/*
    由于 mutex 的存在，多线程并不能并发执行，而是串行的
    因而存在较高的锁竞争，效率比较低

    C1想服务器发送一条消息hello，服务器通过一个 IO 线程转发给所有客户端。
    于此同时 C2 向服务器端发送一条消息 hello2，服务器端通过另一个IO线程
    转发给所有客户端，由于锁的存在，这两个线程并不能并发执行，而是串行的。
    这个时候，客户端数目比较大，第二条消息 hello2 到达各个客户端的延迟也比较大

    降低锁竞争
    锁竞争范围大大减小，提高了并发
    hello 消息到达第一个客户端与最后一个客户端之间的延迟仍然比较大
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

