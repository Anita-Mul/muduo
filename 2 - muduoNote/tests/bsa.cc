#include <boost/static_assert.hpp>

class Timestamp
{
	private:
		int64_t microSecondsSinceEpoch_;
};

// 测试 BOOST_STATIC_ASSERT 运行时断言
// 如果可以编译出来 bsa 文件，说明编译成功
// 如果是下面那种，编译时就会报错
BOOST_STATIC_ASSERT(sizeof(Timestamp) == sizeof(int64_t));
//BOOST_STATIC_ASSERT(sizeof(int) == sizeof(short));

int main(void)
{
	return 0;
}
