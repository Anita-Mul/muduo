## Socket封装
 - Endian.h
封装了字节序转换函数（全局函数，位于muduo::net::sockets名称空间中）。
 - SocketsOps.h/ SocketsOps.cc
封装了socket相关系统调用（全局函数，位于muduo::net::sockets名称空间中）。
 - Socket.h/Socket.cc（Socket类）
用RAII方法封装socket file descriptor
 - InetAddress.h/InetAddress.cc（InetAddress类）
网际地址sockaddr_in封装
