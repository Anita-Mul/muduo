#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>

class Thread
{
	public:
		Thread();		
		// 虚函数		
		virtual ~Thread();

		void Start();
		void Join();

		void SetAutoDelete(bool autoDelete);

	private:
		// 加了静态，就没有隐含的 this 指针了
		static void* ThreadRoutine(void* arg);
		// 纯虚函数
		virtual void Run() = 0;
		pthread_t threadId_;
		bool autoDelete_;
};

#endif // _THREAD_H_
