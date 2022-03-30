#ifndef _THREAD_H_
#define _THREAD_H_

#include <pthread.h>
#include <boost/function.hpp>

// 定义的不是抽象类，是一个具体的类
// 线程的主要目的是执行一个函数，所以这个函数通过类的参数传进来
// 通过 boost 将不同类型的函数统一类型（类型可以是 Thread_test.cpp 里的那些）
class Thread
{
	public:
		// 将类型 boost::function<void ()> 设置为名称 ThreadFunc
		typedef boost::function<void ()> ThreadFunc;
		// 阻止不应该允许的经过转换构造函数进行的隐式转换的发生,声明为explicit的构造函数不能在隐式转换中使用
		explicit Thread(const ThreadFunc& func);

		void Start();
		void Join();

		void SetAutoDelete(bool autoDelete);

	private:
		static void* ThreadRoutine(void* arg);
		void Run();
		ThreadFunc func_;
		pthread_t threadId_;
		bool autoDelete_;
};

#endif // _THREAD_H_
