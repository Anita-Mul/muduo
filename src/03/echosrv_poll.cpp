#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <poll.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <vector>
#include <iostream>

#define ERR_EXIT(m) \
        do \
        { \
                perror(m); \
                exit(EXIT_FAILURE); \
        } while(0)

// 把向量当成数组来用
typedef std::vector<struct pollfd> PollFdList;

/**
 * TIME_WAIT 状态对大并发服务器的影响
 * 
 * 应该尽可能在服务器端避免出现TIME_WAIT状态
 * 如果服务器端主动断开连接（先与client调用close），服务器端就会进入TIME_WAIT
 * 
 * 协议设计上，应该让客户端主动断开连接，这样就把TIME_WAIT状态分散到大量的客户端，
 * 如果客户端不活跃了，一些客户端不断开连接，这样子救会占用服务器端的连接资源
 * 服务器端也要有个机制来踢掉不活跃的连接 close
 */

int main(void)
{
	// 忽略SIGPIPE信号
	// 如果客户端关闭套接字 write，服务器会接收一个 RST segment（TCP 传输层）
	// 如果服务器端再次调用了 write，这个时候就会产生SIGPIPE信号
	signal(SIGPIPE, SIG_IGN);
	// 忽略僵尸进程
	signal(SIGCHLD, SIG_IGN);

	// int idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);
	int listenfd;

	// if ((listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	// SOCK_NONBLOCK 非阻塞套接字
	// SOCK_CLOEXEC fork出的子进程这些文件描述符处于关闭状态
	if ((listenfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)) < 0)
		ERR_EXIT("socket");

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(5188);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int on = 1;
	// 设置端口复用
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		ERR_EXIT("setsockopt");

	if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
		ERR_EXIT("bind");

	if (listen(listenfd, SOMAXCONN) < 0)
		ERR_EXIT("listen");

	struct pollfd pfd;
	pfd.fd = listenfd;
	pfd.events = POLLIN;

	PollFdList pollfds;
	pollfds.push_back(pfd);

	int nready;

	struct sockaddr_in peeraddr;
	socklen_t peerlen;
	int connfd;

	while (1)
	{
		// &*pollfds.begin() 数组首地址
		nready = poll(&*pollfds.begin(), pollfds.size(), -1);

		if (nready == -1)
		{
			if (errno == EINTR)
				continue;
			
			ERR_EXIT("poll");
		}

		if (nready == 0)	// nothing happended
			continue;
		
		// 文件描述符的 POLLIN 事件
		if (pollfds[0].revents & POLLIN)
		{
			peerlen = sizeof(peeraddr);
			connfd = accept4(listenfd, (struct sockaddr*)&peeraddr,
						&peerlen, SOCK_NONBLOCK | SOCK_CLOEXEC);

			if (connfd == -1)
				ERR_EXIT("accept4");

/*
			if (connfd == -1)
			{
				if (errno == EMFILE)
				{
					close(idlefd);
					idlefd = accept(listenfd, NULL, NULL);
					close(idlefd);
					idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);
					continue;
				}
				else
					ERR_EXIT("accept4");
			}
*/

			pfd.fd = connfd;
			pfd.events = POLLIN;
			pfd.revents = 0;
			pollfds.push_back(pfd);
			--nready;

			std::cout<<"ip="<<inet_ntoa(peeraddr.sin_addr)<<
				" port="<<ntohs(peeraddr.sin_port)<<std::endl;
			if (nready == 0)
				continue;
		}

		//std::cout<<pollfds.size()<<std::endl;
		//std::cout<<nready<<std::endl;
		for (PollFdList::iterator it = pollfds.begin() + 1;
			it != pollfds.end() && nready > 0; ++it)
		{
			if (it->revents & POLLIN)
			{
				--nready;
				connfd = it->fd;
				char buf[1024] = {0};
				int ret = read(connfd, buf, 1024);

				if (ret == -1)
					ERR_EXIT("read");
				if (ret == 0)
				{
					std::cout<<"client close"<<std::endl;
					// 向量删除之后会自动将后边的移动到前边
					it = pollfds.erase(it);
					// 如果不 --it，下次 ++it，就会造成漏掉一个
					--it;

					close(connfd);
					continue;
				}

				std::cout<<buf;
				write(connfd, buf, strlen(buf));
			}
		}
	}

	return 0;
}

