#ifndef MUDUO_BASE_TIMESTAMP_H
#define MUDUO_BASE_TIMESTAMP_H

// 这个库包含了这些头文件，所以要加进来
#include <muduo/base/copyable.h>
#include <muduo/base/Types.h>

#include <boost/operators.hpp>

namespace muduo
{
    ///
    /// Time stamp in UTC, in microseconds resolution.
    ///
    /// This class is immutable.
    /// It's recommended to pass it by value, since it's passed in register on x64.
    ///
    class Timestamp : public muduo::copyable,
                      public boost::less_than_comparable<Timestamp>           // 如果实现了 < 操作符，可自动实现 >, <=, >=
    {
        public:
            // 默认初始化时间戳为 0
            Timestamp()
            : microSecondsSinceEpoch_(0)
            {
            }

            // 可以阻止不应该允许的经过转换构造函数进行的隐式转换的发生,声明为explicit的构造函数不能在隐式转换中使用
            // 用户自定义初始化时间戳的值
            explicit Timestamp(int64_t microSecondsSinceEpoch);

            void swap(Timestamp& that)
            {
                std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
            }

            // default copy/assignment/dtor are Okay
            string toString() const;
            string toFormattedString() const;


            bool valid() const { return microSecondsSinceEpoch_ > 0; }


            int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }
            // 转换成秒 kMicroSecondsPerSecond = 1000 * 1000
            // time_t 定义为从格林威治时间1970年01月01日00时00分00秒起至现在的总秒数
            time_t secondsSinceEpoch() const
            { return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond); }

            ///
            /// Get time of now.
            ///
            static Timestamp now();
            static Timestamp invalid();

            static const int kMicroSecondsPerSecond = 1000 * 1000;

        private:
            // 距离 1970-01-01 00:00:00 所流过的微秒数
            int64_t microSecondsSinceEpoch_;
    };


    // inline 指定内联函数
    inline bool operator<(Timestamp lhs, Timestamp rhs)
    {
        return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
    }

    inline bool operator==(Timestamp lhs, Timestamp rhs)
    {
        return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
    }



    /// Gets time difference of two timestamps, result in seconds.
    ///
    /// @param high, low
    /// @return (high-low) in seconds
    inline double timeDifference(Timestamp high, Timestamp low)
    {
        int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
        return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
    }


    /// Add @c seconds to given timestamp.
    ///
    /// @return timestamp+seconds as Timestamp
    inline Timestamp addTime(Timestamp timestamp, double seconds)
    {
        // seconds 转换为微秒
        int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond); 
        return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
    }
}
#endif  // MUDUO_BASE_TIMESTAMP_H
