#include "echo.h"

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <boost/bind.hpp>

#include <assert.h>
#include <stdio.h>

using namespace muduo;
using namespace muduo::net;


EchoServer::EchoServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       int idleSeconds)
  : loop_(loop),
    server_(loop, listenAddr, "EchoServer"),
    connectionBuckets_(idleSeconds)
{
    server_.setConnectionCallback(
        boost::bind(&EchoServer::onConnection, this, _1));
    server_.setMessageCallback(
        boost::bind(&EchoServer::onMessage, this, _1, _2, _3));
    // 注册了一个 1s 钟的定时器
    loop->runEvery(1.0, boost::bind(&EchoServer::onTimer, this));
    connectionBuckets_.resize(idleSeconds);
    dumpConnectionBuckets();
}

void EchoServer::start()
{
    server_.start();
}

void EchoServer::onConnection(const TcpConnectionPtr& conn)
{
    LOG_INFO << "EchoServer - " << conn->peerAddress().toIpPort() << " -> "
            << conn->localAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected())
    {
        // 让一个 entry 和一个 connsction 相关联
        EntryPtr entry(new Entry(conn));
        connectionBuckets_.back().insert(entry);        // 插入到队尾，这时候引用计数为 2
        
        dumpConnectionBuckets();

        // 在 onConnection 的时候保存，当 onMessage 的时候取出，加入环形队列
        // 因为是当客户端不发消息的n秒钟之后断开连接
        WeakEntryPtr weakEntry(entry);                  // 弱引用，引用计数不会加1
        conn->setContext(weakEntry);
    }
    else
    {
        // 当连接断开的时候
        assert(!conn->getContext().empty());
        // 取出这个弱引用
        WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(conn->getContext()));
        LOG_DEBUG << "Entry use_count = " << weakEntry.use_count();
    }
}

void EchoServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp time)
{
    string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name() << " echo " << msg.size()
            << " bytes at " << time.toString();
    conn->send(msg);

    assert(!conn->getContext().empty());
    // 取出这个弱引用
    WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(conn->getContext()));
    // 提升成 sharedptr
    EntryPtr entry(weakEntry.lock());
    if (entry)
    {
        // 插入，使得引用计数加1
        connectionBuckets_.back().insert(entry);
        dumpConnectionBuckets();
    }
}

void EchoServer::onTimer()
{
    // 向前移动一个，并且把格子里面的东西清空（一旦引用计数减为0，Entry的析构函数会被调用）
    // Bucket 就是增加一个空的桶
    connectionBuckets_.push_back(Bucket());
    dumpConnectionBuckets();
}

// 打印桶的引用计数情况
void EchoServer::dumpConnectionBuckets() const
{
    LOG_INFO << "size = " << connectionBuckets_.size();
    int idx = 0;

    for (WeakConnectionList::const_iterator bucketI = connectionBuckets_.begin();
        bucketI != connectionBuckets_.end();
        ++bucketI, ++idx)
    {
        const Bucket& bucket = *bucketI;
        printf("[%d] len = %zd : ", idx, bucket.size());

        for (Bucket::const_iterator it = bucket.begin();
            it != bucket.end();
            ++it)
        {
            bool connectionDead = (*it)->weakConn_.expired();
            printf("%p(%ld)%s, ", get_pointer(*it), it->use_count(),
                connectionDead ? " DEAD" : "");
        }

        puts("");
    }
}

