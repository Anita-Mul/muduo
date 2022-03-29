## Acceptor
 - Acceptor用于accept(2)接受TCP连接
 - Acceptor的数据成员包括Socket、Channel，Acceptor的socket是listening socket（即server socket）。Channel用于观察此socket的readable事件，并回调Accptor::handleRead()，后者调用accept(2)来接受新连接，并回调用户callback。

## Acceptor 示例
```c++
void newConnection(int sockfd, const InetAddress& peerAddr)
{
  printf("newConnection(): accepted a new connection from %s\n",
         peerAddr.toIpPort().c_str());
  ::write(sockfd, "How are you?\n", 13);
  sockets::close(sockfd);
}

int main()
{
  printf("main(): pid = %d\n", getpid());

  InetAddress listenAddr(8888);
  EventLoop loop;

  Acceptor acceptor(&loop, listenAddr);
  acceptor.setNewConnectionCallback(newConnection);
  acceptor.listen();

  loop.loop();
}
```