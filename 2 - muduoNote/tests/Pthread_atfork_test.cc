#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>


/*
	pthread_atfork(void (*prepare)(void），void (*parent)(void）, void(*child)(void))
			prepare在父进程fork创建子进程之前调用；
			child fork返回之前在子进程环境中调用；
			parent fork创建了子进程以后，但在fork返回之前在父进程的进程环境中调用的

			调用fork时，内部创建子进程前在父进程中会调用prepare，内部创建子进程成功后，父进程会调用parent ，子进程会调用child

*/
void prepare(void)
{
	printf("pid = %d prepare ...\n", static_cast<int>(getpid()));
}

void parent(void)
{
	printf("pid = %d parent ...\n", static_cast<int>(getpid()));
}

void child(void)
{
	printf("pid = %d child ...\n", static_cast<int>(getpid()));
}


int main(void)
{
	printf("pid = %d Entering main ...\n", static_cast<int>(getpid()));

	pthread_atfork(prepare, parent, child);

	fork();

	printf("pid = %d Exiting main ...\n",static_cast<int>(getpid()));

	return 0;
}
