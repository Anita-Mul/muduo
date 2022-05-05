// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/net/TcpConnection.h>

#include <muduo/base/Logging.h>
#include <muduo/net/Channel.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/Socket.h>
#include <muduo/net/SocketsOps.h>

#include <boost/bind.hpp>

#include <errno.h>
#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

void muduo::net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    LOG_TRACE << conn->localAddress().toIpPort() << " -> "
                << conn->peerAddress().toIpPort() << " is "
                << (conn->connected() ? "UP" : "DOWN");
}

void muduo::net::defaultMessageCallback(const TcpConnectionPtr&,
                                        Buffer* buf,
                                        Timestamp)
{
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop,
                             const string& nameArg,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
  : loop_(CHECK_NOTNULL(loop)),
    name_(nameArg),
    state_(kConnecting),
    socket_(new Socket(sockfd)),                // new了一个新的socket，对应这条连接
    channel_(new Channel(loop, sockfd)),        // new了一个新的Channel
    localAddr_(localAddr),
    peerAddr_(peerAddr),
    highWaterMark_(64*1024*1024)                // 高水位为64M
{
    // 通道可读事件到来的时候，回调TcpConnection::handleRead，_1是事件发生时间
    channel_->setReadCallback(
        boost::bind(&TcpConnection::handleRead, this, _1));
    // 通道可写事件到来的时候，回调TcpConnection::handleWrite
    channel_->setWriteCallback(
        boost::bind(&TcpConnection::handleWrite, this));
    // 连接关闭，回调TcpConnection::handleClose
    channel_->setCloseCallback(
        boost::bind(&TcpConnection::handleClose, this));
    // 发生错误，回调TcpConnection::handleError
    channel_->setErrorCallback(
        boost::bind(&TcpConnection::handleError, this));
    LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this
              << " fd=" << sockfd;
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
              << " fd=" << channel_->fd();
}

// 线程安全，可以跨线程调用
void TcpConnection::send(const void* data, size_t len)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(data, len);
        }
        else
        {
            string message(static_cast<const char*>(data), len);
            loop_->runInLoop(
                boost::bind(&TcpConnection::sendInLoop,
                            this,
                            message));
        }
    }
}

// 线程安全，可以跨线程调用
void TcpConnection::send(const StringPiece& message)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(message);
        }
        else
        {
            loop_->runInLoop(
                boost::bind(&TcpConnection::sendInLoop,
                            this,
                            message.as_string()));
                          //std::forward<string>(message)));
        }
    }
}

// 线程安全，可以跨线程调用
void TcpConnection::send(Buffer* buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else
        {
            loop_->runInLoop(
                boost::bind(&TcpConnection::sendInLoop,
                            this,
                            buf->retrieveAllAsString()));
                          //std::forward<string>(message)));
        }
    }
}

void TcpConnection::sendInLoop(const StringPiece& message)
{
    sendInLoop(message.data(), message.size());
}

void TcpConnection::sendInLoop(const void* data, size_t len)
{
    /*
    loop_->assertInLoopThread();
    sockets::write(channel_->fd(), data, len);
    */

    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool error = false;

    // 如果状态是断开连接
    if (state_ == kDisconnected)
    {
        LOG_WARN << "disconnected, give up writing";
        return;
    }

    // if no thing in output queue, try writing directly
    // 通道没有关注可写事件并且发送缓冲区没有数据，直接write
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = sockets::write(channel_->fd(), data, len);

        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            // 写完了，回调writeCompleteCallback_
            if (remaining == 0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else // nwrote < 0    发生错误
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_SYSERR << "TcpConnection::sendInLoop";
                if (errno == EPIPE) // FIXME: any others?
                {
                    error = true;
                }
            }
        }
    }
    assert(remaining <= len);
    
    // 没有错误，并且还有未写完的数据（说明内核发送缓冲区满，要将未写完的数据添加到output buffer中）
    if (!error && remaining > 0)
    {
      LOG_TRACE << "I am going to write more data";
      size_t oldLen = outputBuffer_.readableBytes();

      // 如果超过highWaterMark_（高水位标，缓冲区的极限），回调highWaterMarkCallback_
      if (oldLen + remaining >= highWaterMark_     // 构造函数中设置成了 64M
          && oldLen < highWaterMark_
          && highWaterMarkCallback_)
      {
          loop_->queueInLoop(boost::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
      }

      outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
      
      if (!channel_->isWriting())
      {
          channel_->enableWriting();		// 关注POLLOUT事件
      }
    }
}

// 先清空缓冲区的数据，然后再关闭
// 服务器端主动断开与客户端的连接
// 这意味着客户端 read 返回为 0
void TcpConnection::shutdown()
{
    // FIXME: use compare and swap
    if (state_ == kConnected)
    {
        // 更改状态为正在关闭
        // 如果缓冲区中还没有数据发送完，只是将连接状态更改为 kDisconnecting，并没有关闭连接
        setState(kDisconnecting);                   
        // FIXME: shared_from_this()?
        loop_->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();

    if (!channel_->isWriting())
    {
        // we are not writing
        // 只关闭写的这一半
        socket_->shutdownWrite();
    }
}

void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

// 设置已连接状态
void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);

    setState(kConnected);
    LOG_TRACE << "[3] usecount=" << shared_from_this().use_count();

    channel_->tie(shared_from_this());
    channel_->enableReading();	// TcpConnection所对应的通道加入到Poller关注

    connectionCallback_(shared_from_this());
    LOG_TRACE << "[4] usecount=" << shared_from_this().use_count();
}

// 设置已关闭连接状态
void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();

    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }
    
    channel_->remove();
}


void TcpConnection::handleRead(Timestamp receiveTime)
{
    /*
    loop_->assertInLoopThread();
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
      messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
      handleClose();
    }
    else
    {
      errno = savedErrno;
      LOG_SYSERR << "TcpConnection::handleRead";
      handleError();
    }
    */

    /*
    loop_->assertInLoopThread();
    int savedErrno = 0;
    char buf[65536];
    ssize_t n = ::read(channel_->fd(), buf, sizeof buf);
    if (n > 0)
    {
      messageCallback_(shared_from_this(), buf, n);
    }
    else if (n == 0)
    {
      handleClose();
    }
    else
    {
      errno = savedErrno;
      LOG_SYSERR << "TcpConnection::handleRead";
      handleError();
    }
    */
    loop_->assertInLoopThread();
    int savedErrno = 0;
    // 从文件描述符 fd 中读取数据，存储到 inputBuffer 中，LT 触发模式
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);

    if (n > 0)
    {
        // 喊客户代码来拿数据了
        // 消息到来的回调函数
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }
}

// 内核发送缓冲区有空间了，回调该函数
// 当 socket 可写，调用这个函数
void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();

    if (channel_->isWriting())
    {
        // 用户将要写的内容写到 outputBuffer
        // 向socket中写入 outputBuffer_.readableBytes个字节的buffer内的内容
        ssize_t n = sockets::write(channel_->fd(),
                                  outputBuffer_.peek(),                 // 返回读的指针
                                  outputBuffer_.readableBytes());       // 可读的字节数
        if (n > 0)
        {
            outputBuffer_.retrieve(n);                  // buffer释放n个字节的空间，也不能说释放，buffer是通过移动指针来操作区间的

            if (outputBuffer_.readableBytes() == 0)	    // 发送缓冲区已清空，没有数据了
            {
              channel_->disableWriting();		        // 停止关注POLLOUT事件，以免出现busy loop
                                                        // 因为目的就是关注 POLLOUT 事件的目的就是将 buffer 中的数据全写入缓冲区，既然全写入了，就不用关注了
              if (writeCompleteCallback_)		        // 回调writeCompleteCallback_
              {
                  // 应用层发送缓冲区被清空，就回调用writeCompleteCallback_
                  loop_->queueInLoop(boost::bind(writeCompleteCallback_, shared_from_this()));
              }

              if (state_ == kDisconnecting)	            // 发送缓冲区已清空并且连接状态是kDisconnecting, 要关闭连接
              {
                shutdownInLoop();		                // 关闭连接
              }
            }
            else
            {
                LOG_TRACE << "I am going to write more data";
            }
        }
        else
        {
          LOG_SYSERR << "TcpConnection::handleWrite";
          // if (state_ == kDisconnecting)
          // {
          //   shutdownInLoop();
          // }
        }
    }
    else
    {
      LOG_TRACE << "Connection fd = " << channel_->fd()
                << " is down, no more writing";
    }
}

// 流程是，输出日志，设置状态，关闭通道，通知客户代码关闭连接，最后调用TcpConnection的销毁连接代码
void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();

    LOG_TRACE << "fd = " << channel_->fd() << " state = " << state_;
    assert(state_ == kConnected || state_ == kDisconnecting);

    // we don't close fd, leave it to dtor, so we can find leaks easily.
    setState(kDisconnected);            // 设置状态为切断的
    // 关闭这个文件描述符上的所有读写错误事件
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);		//这里跟建立连接时一样，再次调用了这个回调函数，所以在用户代码中，需要进行分情况，看到底时建立连接还是断开连接
    LOG_TRACE << "[7] usecount=" << guardThis.use_count();
    // must be the last line
    closeCallback_(guardThis);	        // 调用TcpServer::removeConnection
    LOG_TRACE << "[11] usecount=" << guardThis.use_count();
}

void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}
