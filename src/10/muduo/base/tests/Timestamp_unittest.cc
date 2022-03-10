#include <muduo/base/Timestamp.h>
#include <vector>
#include <stdio.h>

using muduo::Timestamp;

// 引用传递
void passByConstReference(const Timestamp& x)
{
  printf("%s\n", x.toString().c_str());
}

// 值传递
void passByValue(Timestamp x)
{
  printf("%s\n", x.toString().c_str());
}

// 度量时间的函数
void benchmark()
{
  const int kNumber = 1000*1000;

  std::vector<Timestamp> stamps;
  // 预留空间
  stamps.reserve(kNumber);


  for (int i = 0; i < kNumber; ++i)
  {
    // 消耗的时间主要在 Timestamp::now()
    // stamps 已经预留好空间了
    stamps.push_back(Timestamp::now());
  }
  printf("%s\n", stamps.front().toString().c_str());
  printf("%s\n", stamps.back().toString().c_str());
  printf("%f\n", timeDifference(stamps.back(), stamps.front()));


  int increments[100] = { 0 };
  int64_t start = stamps.front().microSecondsSinceEpoch();
  for (int i = 1; i < kNumber; ++i)
  {
    int64_t next = stamps[i].microSecondsSinceEpoch();
    int64_t inc = next - start;
    start = next;
    if (inc < 0)
    {
      // 这种情况一般是不可能的，如果出现了这种情况，可能是硬件方面的问题
      printf("reverse!\n");
    }
    else if (inc < 100)
    {
      // 将小于 100 的个数 ++
      ++increments[inc];
    }
    else
    {
      printf("big gap %d\n", static_cast<int>(inc));
    }
  }

  for (int i = 0; i < 100; ++i)
  {
    printf("%2d: %d\n", i, increments[i]);
  }
}

int main()
{
  // 构造一个时间戳对象
  Timestamp now(Timestamp::now());
  printf("%s\n", now.toString().c_str());
  passByValue(now);
  passByConstReference(now);
  benchmark();
}

