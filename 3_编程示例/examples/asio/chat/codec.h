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
�������Ϣ
�������δ�������������Ϣ��
�������һ��Ӧ�ò��У����Ϣ�ġ�����˵ CRC32 У��
�ڶ��ַ�ʽ
��������Ӧ���п��жϿ����ܡ���һ��ʱ��û���յ��ͻ��˵���Ϣ���ͶϿ���
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
        
        // ������Ϣ������TcpServer �Ļص�����
        void onMessage(const muduo::net::TcpConnectionPtr& conn,
                        muduo::net::Buffer* buf,
                        muduo::Timestamp receiveTime)
        {
            // ������while������if
            while (buf->readableBytes() >= kHeaderLen)                              // kHeaderLen == 4
            {
                // FIXME: use Buffer::peekInt32()
                const void* data = buf->peek();                                     // ���ض���ָ��
                int32_t be32 = *static_cast<const int32_t*>(data);                  // SIGBUS
                const int32_t len = muduo::net::sockets::networkToHost32(be32);     // ���ݲ��ֵĳ���

                if (len > 65536 || len < 0)
                {
                    LOG_ERROR << "Invalid length " << len;
                    conn->shutdown();  // FIXME: disable reading
                    break;
                }
                else if (buf->readableBytes() >= len + kHeaderLen)                  // �ﵽһ����������Ϣ
                {   
                    // ɾ��������ͷ�����ݵ�����
                    buf->retrieve(kHeaderLen);
                    // ȡ�����������ݲ��ֵ�����
                    muduo::string message(buf->peek(), len);
                    // ���ûص�����
                    messageCallback_(conn, message, receiveTime);
                    // ɾ�����������ݲ��ֵ�����
                    buf->retrieve(len);
                }
                else  // δ�ﵽһ����������Ϣ
                {
                    // ճ��
                    // TCP ���ֽ��������ޱ߽��
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
        // �����Ļص�����
        StringMessageCallback messageCallback_;
        const static size_t kHeaderLen = sizeof(int32_t);
};

#endif  // MUDUO_EXAMPLES_ASIO_CHAT_CODEC_H
