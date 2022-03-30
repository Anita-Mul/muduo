#ifndef MUDUO_EXAMPLES_ASIO_CHAT_CODEC_H
#define MUDUO_EXAMPLES_ASIO_CHAT_CODEC_H

#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Endian.h>
#include <muduo/net/TcpConnection.h>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

/*
0x00, 0x00, 0x00, 0x08, 'h', 'e', 'l', 'l', 'o'
错误的消息
服务端如何处理这个错误的消息呢
还会带上一个应用层的校验信息的。比如说 CRC32 校验
第二种方式
服务器端应该有空闲断开功能。在一定时间没有收到客户端的消息，就断开它
*/
class LengthHeaderCodec : boost::noncopyable
{
    public:
        typedef boost::function<void (const muduo::net::TcpConnectionPtr&,
                                        const muduo::string& message,
                                        muduo::Timestamp)> StringMessageCallback;

        explicit LengthHeaderCodec(const StringMessageCallback& cb)
            : messageCallback_(cb)
        {
        }
        
        // 当有消息到来，TcpServer 的回调函数
        void onMessage(const muduo::net::TcpConnectionPtr& conn,
                        muduo::net::Buffer* buf,
                        muduo::Timestamp receiveTime)
        {
            // 这里用while而不用if
            while (buf->readableBytes() >= kHeaderLen)                              // kHeaderLen == 4
            {
                // FIXME: use Buffer::peekInt32()
                const void* data = buf->peek();                                     // 返回读的指针
                int32_t be32 = *static_cast<const int32_t*>(data);                  // SIGBUS
                const int32_t len = muduo::net::sockets::networkToHost32(be32);     // 数据部分的长度

                if (len > 65536 || len < 0)
                {
                    LOG_ERROR << "Invalid length " << len;
                    conn->shutdown();  // FIXME: disable reading
                    break;
                }
                else if (buf->readableBytes() >= len + kHeaderLen)                  // 达到一条完整的消息
                {   
                    // 删除缓冲区头部数据的区域
                    buf->retrieve(kHeaderLen);
                    // 取出缓冲区数据部分的区域
                    muduo::string message(buf->peek(), len);
                    // 调用回调函数
                    messageCallback_(conn, message, receiveTime);
                    // 删除缓冲区数据部分的区域
                    buf->retrieve(len);
                }
                else  // 未达到一条完整的消息
                {
                    // 粘包
                    // TCP 是字节流，是无边界的
                    break;
                }
            }
        }

        // FIXME: TcpConnectionPtr
        void send(muduo::net::TcpConnection* conn,
                    const muduo::StringPiece& message)
        {
            muduo::net::Buffer buf;
            buf.append(message.data(), message.size());

            int32_t len = static_cast<int32_t>(message.size());
            int32_t be32 = muduo::net::sockets::hostToNetwork32(len);
            buf.prepend(&be32, sizeof be32);

            conn->send(&buf);
        }

    private:
        // 传来的回调函数
        StringMessageCallback messageCallback_;
        const static size_t kHeaderLen = sizeof(int32_t);
};

#endif  // MUDUO_EXAMPLES_ASIO_CHAT_CODEC_H
