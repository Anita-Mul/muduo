// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include <muduo/net/Channel.h>
#include <muduo/net/Socket.h>

namespace muduo
{
    namespace net
    {
        class EventLoop;
        class InetAddress;

        ///
        /// Acceptor of incoming TCP connections.
        /// Acceptor用于接受（accept）客户端的连接，通过设置回调函数通知使用者。
        /// 它只在muduo网络库内部的TcpServer使用，由TcpServer控制它的生命期
        ///
        class Acceptor : boost::noncopyable
        {
            public:
                typedef boost::function<void (int sockfd,
                                              const InetAddress&)> NewConnectionCallback;

                Acceptor(EventLoop* loop, const InetAddress& listenAddr);
                ~Acceptor();

                void setNewConnectionCallback(const NewConnectionCallback& cb)
                { newConnectionCallback_ = cb; }

                bool listenning() const { return listenning_; }
                void listen();

            private:
                void handleRead();                              // 可读回调函数

                EventLoop* loop_;
                Socket acceptSocket_;                           // 监听套接字
                Channel acceptChannel_;                         // 监听的通道
                NewConnectionCallback newConnectionCallback_;   // 一旦有新连接发生执行的回调函数
                bool listenning_;                               // acceptChannel所处的EventLoop是否处于监听状态
                int idleFd_;                                    // 用来解决文件描述符过多引起电平触发不断触发的问题
        };  
    }
}

#endif  // MUDUO_NET_ACCEPTOR_H
