#include <iostream>
#include <boost/function.hpp>
#include <boost/bind.hpp>
using namespace std;

class Foo
{
	public:
		// 实际有四个参数
		// this    d   i   j
		// &foo  0.5  _1  _2

		void memberFunc(double d, int i, int j)
		{
			cout << d << endl;//打印0.5
			cout << i << endl;//打印100       
			cout << j << endl;//打印10
		}
};

int main()
{
	Foo foo;
	// 适配为 function<void (int, int)>   =>  void f(int, int)
	// _1 _2 _3 表示占位符
	// 相当于 (&foo) -> memberFunc(...)
	boost::function<void (int, int)> fp = boost::bind(&Foo::memberFunc, &foo, 0.5, _1, _2);
	fp(100, 200);
	// boost::ref(foo) 表示是一种引用
	// 相当于 foo.memberFunc()
	boost::function<void (int, int)> fp2 = boost::bind(&Foo::memberFunc, boost::ref(foo), 0.5, _1, _2);
	fp2(55, 66);
	return 0;
}

/**
 * 0.5
 * 100
 * 200
 * 0.5
 * 55
 * 66
 */

