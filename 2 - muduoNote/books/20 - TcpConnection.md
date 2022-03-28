## TcpConnection生存期管理
 - 当连接到来，创建一个 TcpConnection 对象，立刻用 shared_ptr来管理，引用计数为1，在 Channel 中维护一个 weak_ptr(tie_)，将这个 shared_ptr 对象赋值给 tie_，引用计数仍为1
 - 当连接关闭，在 handleEvent，将 tie_ 提升，得到一个 shared_ptr 对象，引用计数就变成了 2

## 完善TcpConnection
WriteCompleteCallback含义
HighWaterMarkCallback含义
boost::any context_
## signal(SIGPIPE, SIG_IGN)
## 可变类型解决方案
 - void*. 这种方法不是类型安全的
 - boost::any
## boost::any
 - 任意类型的类型安全存储以及安全的取回
 - 在标准库容器中存放不同类型的方法，比如说vector<boost::any>

## 笔记
###### 大流量
 - 不断生成数据，然后发送 conn -> send()
 - 如果对邓发接收不及时，受到通告窗口的控制，内核发送缓冲不足，这个时候，就会将用户数据添加到应用层发送缓冲区（output buffer），可能会撑爆 output buffer
 - 解决方法就是，调整发送频率
 - 关注 WriteCompleteCallback
 - 所有数据都发送完，WriteCompleteCallback回调，然后继续发送