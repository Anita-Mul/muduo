// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/Acceptor.h>

#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <fcntl.h>
//#include <sys/types.h>
//#include <sys/stat.h>

using namespace muduo;
using namespace muduo::net;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
  : loop_(loop),
    acceptSocket_(sockets::createNonblockingOrDie()),       // 创建监听套接字
    acceptChannel_(loop, acceptSocket_.fd()),
    listenning_(false),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))      // 预先准备一个空闲文件描述符
{
    assert(idleFd_ >= 0);
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback(
        boost::bind(&Acceptor::handleRead, this));          // 设置Channel的fd的读回调函数
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    ::close(idleFd_);
}

// Acceptor是在listen中开始关注可读事件
void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

void Acceptor::handleRead()
{
    loop_->assertInLoopThread();
    // 对等方的地址
    InetAddress peerAddr(0);

    //FIXME loop until no more
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        // string hostport = peerAddr.toIpPort();
        // LOG_TRACE << "Accepts of " << hostport;
        if (newConnectionCallback_)                         // 如果设置了新连接回调函数
        {
            // 调用回调函数
            newConnectionCallback_(connfd, peerAddr);       // 那么就执行它
        }
        else
        {
            sockets::close(connfd);                         // 否则就关闭，sockets是全局函数
        }
    }
    else
    {
      // Read the section named "The special problem of
      // accept()ing when you can't" in libev's doc.
      // By Marc Lehmann, author of livev.
        if (errno == EMFILE)        // 文件描述符太多了
        {
            // 先关闭空闲文件描述符，让它能够接收。否则由于采用电平触发，不接收会一直触发。
            ::close(idleFd_);   
            // 那就腾出一个文件描述符，用来accept
            idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
            // accept之后再关闭
            ::close(idleFd_);
            // 然后再打开成默认方式
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}

