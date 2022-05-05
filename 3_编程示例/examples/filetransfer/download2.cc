#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

/**
 * 注意每次建立连接的时候我们都去重新打开那个文件，使得程序中文件描述符的数量翻倍（每个连接占一个socket fd和一个file fd），
 * 这是考虑到文件有可能被其他程序修改。如果文件是immutable的，一种改进措施是：整个程序可以共享同一个文件描述符，然后每个连
 * 接记住自己当前的偏移量，在onWriteComplete()回调函数里用pread(2)来读取数据
 */
void onHighWaterMark(const TcpConnectionPtr& conn, size_t len)
{
    LOG_INFO << "HighWaterMark " << len;
}

const int kBufSize = 64*1024;
const char* g_file = NULL;

void onConnection(const TcpConnectionPtr& conn)
{
    LOG_INFO << "FileServer - " << conn->peerAddress().toIpPort() << " -> "
            << conn->localAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
    
    if (conn->connected())
    {
        LOG_INFO << "FileServer - Sending file " << g_file
                << " to " << conn->peerAddress().toIpPort();
        conn->setHighWaterMarkCallback(onHighWaterMark, kBufSize+1);

        FILE* fp = ::fopen(g_file, "rb");

        if (fp)
        {
            // 将 TcpConnection 对象与 fp 绑定
            // 通过这种方法，我们就不需要额外再用一个 map 容器来管理对应关系
            conn->setContext(fp);
            char buf[kBufSize];
            size_t nread = ::fread(buf, 1, sizeof buf, fp);
            conn->send(buf, nread);
        }
        else
        {
            conn->shutdown();
            LOG_INFO << "FileServer - no such file";
        }
    }
    else
    {
        if (!conn->getContext().empty())
        {
            FILE* fp = boost::any_cast<FILE*>(conn->getContext());
            if (fp)
            {
                ::fclose(fp);
            }
        }
    }
}

// 在onWriteComplete()回调函数中读取下一块文件数据，继续发送。
void onWriteComplete(const TcpConnectionPtr& conn)
{
    FILE* fp = boost::any_cast<FILE*>(conn->getContext());
    char buf[kBufSize];
    size_t nread = ::fread(buf, 1, sizeof buf, fp);

    if (nread > 0)
    {
        conn->send(buf, nread);
    }
    else
    {
        ::fclose(fp);
        fp = NULL;
        conn->setContext(fp);
        conn->shutdown();
        LOG_INFO << "FileServer - done";
    }
}


/*
一块一块地发送文件，减少内存使用，用到了 WriteCompleteCallback，这个
版本满足了上述全部健壮性要求
*/
int main(int argc, char* argv[])
{
    LOG_INFO << "pid = " << getpid();

    if (argc > 1)
    {
        g_file = argv[1];

        EventLoop loop;
        InetAddress listenAddr(2021);
        TcpServer server(&loop, listenAddr, "FileServer");
        server.setConnectionCallback(onConnection);
        server.setWriteCompleteCallback(onWriteComplete);
        server.start();
        loop.loop();
    }
    else
    {
        fprintf(stderr, "Usage: %s file_for_downloading\n", argv[0]);
    }
}

