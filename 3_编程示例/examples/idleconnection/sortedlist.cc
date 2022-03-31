#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <boost/bind.hpp>
#include <list>
#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

// RFC 862
// 用排序过的链表来实现踢掉空闲连接
class EchoServer
{
    public:
        EchoServer(EventLoop* loop,
                  const InetAddress& listenAddr,
                  int idleSeconds);

        void start()
        {
            server_.start();
        }

    private:
        void onConnection(const TcpConnectionPtr& conn);

        void onMessage(const TcpConnectionPtr& conn,
                      Buffer* buf,
                      Timestamp time);

        void onTimer();

        void dumpConnectionList() const;

        typedef boost::weak_ptr<TcpConnection> WeakTcpConnectionPtr;
        typedef std::list<WeakTcpConnectionPtr> WeakConnectionList;

        struct Node : public muduo::copyable
        {
            Timestamp lastReceiveTime;                    // 该连接最后一次活跃时刻
            WeakConnectionList::iterator position;        // 该连接在连接列表中的位置 
        };  

        EventLoop* loop_; 
        TcpServer server_;
        int idleSeconds_;
        WeakConnectionList connectionList_;               // 连接列表
};

EchoServer::EchoServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       int idleSeconds)
  : loop_(loop),
    server_(loop, listenAddr, "EchoServer"),
    idleSeconds_(idleSeconds)
{
    server_.setConnectionCallback(
        boost::bind(&EchoServer::onConnection, this, _1));
    server_.setMessageCallback(
        boost::bind(&EchoServer::onMessage, this, _1, _2, _3));
    
    // 注册一个 1s 钟的定时器
    loop->runEvery(1.0, boost::bind(&EchoServer::onTimer, this));
    dumpConnectionList();
}

void EchoServer::onConnection(const TcpConnectionPtr& conn)
{
    LOG_INFO << "EchoServer - " << conn->peerAddress().toIpPort() << " -> "
            << conn->localAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected())
    {
        Node node;
        node.lastReceiveTime = Timestamp::now();      // 该连接最后一次活跃时刻
        connectionList_.push_back(conn);
        node.position = --connectionList_.end();      // 该连接在连接列表中的位置 
        
        conn->setContext(node);
    }
    else
    {
        // 清除 connection 在 connectionList_ 中的位置
        assert(!conn->getContext().empty());
        const Node& node = boost::any_cast<const Node&>(conn->getContext());
        connectionList_.erase(node.position);
    }

    dumpConnectionList();
}

void EchoServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp time)
{
    // 当一个消息到来，说明它的生存期又延长了
    string msg(buf->retrieveAllAsString());
    LOG_INFO << conn->name() << " echo " << msg.size()
            << " bytes at " << time.toString();
    conn->send(msg);

    assert(!conn->getContext().empty());
    Node* node = boost::any_cast<Node>(conn->getMutableContext());
    node->lastReceiveTime = time;
    // move node inside list with list::splice()
    // 时间更新了，需要将连接移至列表末端，以保证列表是按最后更新时刻排序
    connectionList_.erase(node->position);
    connectionList_.push_back(conn);
    node->position = --connectionList_.end();

    dumpConnectionList();
}

void EchoServer::onTimer()
{
    dumpConnectionList();
    Timestamp now = Timestamp::now();

    for (WeakConnectionList::iterator it = connectionList_.begin();
        it != connectionList_.end();)
    {
        TcpConnectionPtr conn = it->lock();
        if (conn)
        {
            Node* n = boost::any_cast<Node>(conn->getMutableContext());
            double age = timeDifference(now, n->lastReceiveTime);
            
            if (age > idleSeconds_)             // 说明超时了
            {
                conn->shutdown();
            }
            else if (age < 0)                   // 这种情况一般不可能发生
            {
                LOG_WARN << "Time jump";
                n->lastReceiveTime = now;
            }
            else                                // age >= 0 且 age <= idleSeconds_，说明未超时
            {   
                // 只遍历一部分，后面的就不需要再遍历了
                break;
            }
            
            ++it;
        }
        else                                    // 说明连接已关闭
        {
            LOG_WARN << "Expired";
            it = connectionList_.erase(it);
        }
    }
}

void EchoServer::dumpConnectionList() const
{
    LOG_INFO << "size = " << connectionList_.size();

    for (WeakConnectionList::const_iterator it = connectionList_.begin();
        it != connectionList_.end(); ++it)
    {
        TcpConnectionPtr conn = it->lock();

        if (conn)
        {
            printf("conn %p\n", get_pointer(conn));
            const Node& n = boost::any_cast<const Node&>(conn->getContext());
            printf("    time %s\n", n.lastReceiveTime.toString().c_str());
        }
        else
        {
            printf("expired\n");
        }
    }
}

int main(int argc, char* argv[])
{
    EventLoop loop;
    InetAddress listenAddr(2007);
    int idleSeconds = 10;

    if (argc > 1)
    {
        idleSeconds = atoi(argv[1]);
    }

    LOG_INFO << "pid = " << getpid() << ", idle seconds = " << idleSeconds;
    EchoServer server(&loop, listenAddr, idleSeconds);
    server.start();
    loop.loop();
}

