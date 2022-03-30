#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

const char* g_file = NULL;

// FIXME: use FileUtil::readFile()
string readFile(const char* filename)
{
    string content;
    FILE* fp = ::fopen(filename, "rb");

    if (fp)
    {
        // inefficient!!!
        const int kBufSize = 1024*1024;
        char iobuf[kBufSize];
        ::setbuffer(fp, iobuf, sizeof iobuf);

        char buf[kBufSize];
        size_t nread = 0;

        while ( (nread = ::fread(buf, 1, sizeof buf, fp)) > 0)
        {
            content.append(buf, nread);
        }

        ::fclose(fp);
    }

    return content;
}

void onHighWaterMark(const TcpConnectionPtr& conn, size_t len)
{
    LOG_INFO << "HighWaterMark " << len;
}

void onConnection(const TcpConnectionPtr& conn)
{
    LOG_INFO << "FileServer - " << conn->peerAddress().toIpPort() << " -> "
            << conn->localAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
    
    if (conn->connected())
    {
        LOG_INFO << "FileServer - Sending file " << g_file
                << " to " << conn->peerAddress().toIpPort();
        conn->setHighWaterMarkCallback(onHighWaterMark, 64*1024);
        string fileContent = readFile(g_file);
        /*
          send 函数是非阻塞的，立刻返回，不用担心数据什么时候给对等方
          这个由网络库负责到底

          fileContent 比较大的时候，是没有办法一次性将数据拷到内核缓冲区的
          这时候，会将剩余的数据拷贝到应用层的 OutputBuffer 中。
          当内核缓冲区中的数据发送出去之后，可写事件产生，muduo就会从 OutputBuffer 中
          取出数据发送

          直接调用 shutdown 函数，会将状态改为 kDisconnecting，会检查 channel -> isWriting 的状态
          在 handleWrite 函数中
          if (state_ == kDisconnecting)	            // 发送缓冲区已清空并且连接状态是kDisconnecting, 要关闭连接
          {
            shutdownInLoop();		                // 关闭连接
          }
        */
        conn->send(fileContent);
        conn->shutdown();
        LOG_INFO << "FileServer - done";
    }
}

/*
一次性把文件读入内存，一次性调用 send(const string&) 发送完毕，这个版本
满足除了“内存消耗只能并发连接数有关，跟文件大小无关”之外的健壮性要求
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
        server.start();
        loop.loop();
    }
    else
    {
        fprintf(stderr, "Usage: %s file_for_downloading\n", argv[0]);
    }
}

