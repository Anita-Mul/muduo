## 五个简单 TCP 协议 simle/
 - discard - 丢弃所有收到的数据；
 - daytime - 服务端 accept 连接之后，以字符串形式发送当前时间，然后主动断
开连接；
 - time - 服务端 accept 连接之后，以二进制形式发送当前时间（从 Epoch 到现在
的秒数），然后主动断开连接；我们需要一个客户程序来把收到的时间转换为字
符串。
 - echo - 回显服务，把收到的数据发回客户端；
 - chargen - 服务端 accept 连接之后，不停地发送测试数据。

## muduo 库网络模型使用示例（sudoku 求解服务器 MuduoManual.pdf P35）
 - reactor（一个IO线程）
 - multiple reactor （多个IO线程）
 - one loop per thread + thread pool （多个IO线程 + 计算线程池）

## 文件传输（MuduoManual.pdf P57）
 - examples/filetransfer/download.cc
 - examples/filetransfer/download2.cc
 - examples/filetransfer/download3.cc
 - tests/Filetransfer_test.cc

