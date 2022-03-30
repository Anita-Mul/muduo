## 定时器
 - muduo的定时器由三个类实现，TimerId、Timer、TimerQueue，用户只能看到第一个类，其它两个都是内部实现细节
 - TimerQueue的接口很简单，只有两个函数addTimer和cancel
 - EventLoop
    > runAt		    在某个时刻运行定时器
    > runAfter		过一段时间运行定时器
    > runEvery		每隔一段时间运行定时器
    > cancel		取消定时器
 - TimerQueue数据结构的选择，能快速根据当前时间找到已到期的定时器，也要高效的添加和删除Timer，因而可以用二叉搜索树，用map或者set
    > typedef std::pair<Timestamp, Timer*> Entry;
    > typedef std::set<Entry> TimerList;

## lower_bound&upper_bound
```c++
#include <set>
#include <iostream>

using namespace std;

int main(void)
{
	int a[] = { 1, 2, 3, 4, 5 };
	set<int> s(a, a+5);

	cout<<*s.lower_bound(2)<<endl;
	cout<<*s.upper_bound(2)<<endl;
	return 0;
}
```

## RVO
```c++
struct Foo   
{   
	Foo() { cout << "Foo ctor" << endl; }
	Foo(const Foo&) { cout << "Foo copy ctor" << endl; }
	void operator=(const Foo&) { cout << "Foo operator=" << endl; } 
	~Foo() { cout << "Foo dtor" << endl; }
};  
Foo make_foo()   
{
	//return Foo();
	Foo f;
	return f;
}  
int main(void)
{
	make_foo();
	return 0;
}
```