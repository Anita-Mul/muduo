#include <stdio.h>


/**
 * 测试编译选项
 * 		g++ test.cpp
 * 		g++  -Wall test.cpp
 * 		...
 */
void foo(int x)
{
}

class B
{
	public:
		virtual void foo()
		{
		}
};

class D : public B
{
	public:
		void foo(int x)
		{
		}
};

// 具体实现在 Types.h 中
// 同时还有一个向下转型
template<typename To, typename From>
inline To implicit_cast(From const &f) {
  return f;
}

int main(void)
{
	int n;
	double d = 1.23;
	n = d;

	B* pb;
	D* pd = NULL;

	pb = pd;
	// 用implicit_cast可以实现 一种 static_cast 或者 const_cast在向上转型时的安全版本
	pb = implicit_cast<B*, D*>(pd);
	return 0;
}
