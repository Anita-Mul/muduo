#ifndef MUDUO_EXAMPLES_IDLECONNECTION_ECHO_H
#define MUDUO_EXAMPLES_IDLECONNECTION_ECHO_H

#include <muduo/net/TcpServer.h>
//#include <muduo/base/Types.h>

#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>

#if BOOST_VERSION < 104700
namespace boost
{
    template <typename T>
    inline size_t hash_value(const boost::shared_ptr<T>& x)
    {
        return boost::hash_value(x.get());
    }
}
#endif

// RFC 862
class EchoServer
{
    public:
        EchoServer(muduo::net::EventLoop* loop,
                    const muduo::net::InetAddress& listenAddr,
                    int idleSeconds);

        void start();

    private:
        void onConnection(const muduo::net::TcpConnectionPtr& conn);

        void onMessage(const muduo::net::TcpConnectionPtr& conn,
                        muduo::net::Buffer* buf,
                        muduo::Timestamp time);

        void onTimer();

        void dumpConnectionBuckets() const;

        typedef boost::weak_ptr<muduo::net::TcpConnection> WeakTcpConnectionPtr;

        struct Entry : public muduo::copyable
        {
            explicit Entry(const WeakTcpConnectionPtr& weakConn)
                : weakConn_(weakConn)
            {
            }

            ~Entry()
            {
                // 一旦引用计数减为0，Entry的析构函数会被调用
                muduo::net::TcpConnectionPtr conn = weakConn_.lock();
                if (conn)
                {
                    conn->shutdown();
                }
            }

            WeakTcpConnectionPtr weakConn_;
        };

        typedef boost::shared_ptr<Entry> EntryPtr;                    // set 中的元素是一个 EntryPtr
        typedef boost::weak_ptr<Entry> WeakEntryPtr;
        // 普通的 set 当我们把元素放进去的时候会自动排序
        // unordered_set 不会自动排序
        // 队尾，每隔 1s 钟向前移动一格
        // 如果在 1s 钟之内，队尾 bucket 中的连接收到消息，这时候不需要增加连接所对应的 entry
        // set 会去除重复的
        typedef boost::unordered_set<EntryPtr> Bucket;                // 环形缓冲区每个格子存放的是一个 hash_set
        typedef boost::circular_buffer<Bucket> WeakConnectionList;    // 环形缓冲区

        muduo::net::EventLoop* loop_;
        muduo::net::TcpServer server_;
        WeakConnectionList connectionBuckets_;                        // 连接队列环形缓冲区
};

#endif  // MUDUO_EXAMPLES_IDLECONNECTION_ECHO_H
