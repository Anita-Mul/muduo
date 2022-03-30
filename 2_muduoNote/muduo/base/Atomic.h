// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_ATOMIC_H
#define MUDUO_BASE_ATOMIC_H

#include <boost/noncopyable.hpp>
#include <stdint.h>

namespace muduo
{
    namespace detail
    {
        // 函数模板
        // 可以看到下面是这样设置的 typedef detail::AtomicIntegerT<int32_t> AtomicInt32;
        template<typename T>
        // boost::noncopyable 是不可以拷贝的
        class AtomicIntegerT : boost::noncopyable
        {
            public:
                AtomicIntegerT()
                  : value_(0)
                {
                }

                // uncomment if you need copying and assignment
                //
                // AtomicIntegerT(const AtomicIntegerT& that)
                //   : value_(that.get())
                // {}
                //
                // AtomicIntegerT& operator=(const AtomicIntegerT& that)
                // {
                //   getAndSet(that.get());
                //   return *this;
                // }

                T get()
                {   
                    // 比较当前 value_ 的值是否等于0，如果等于0，就把 value_ 的值设置为 0
                    // 返回的是 value 修改之前的值
                    return __sync_val_compare_and_swap(&value_, 0, 0);
                }

                T getAndAdd(T x)
                {
                    // value_ 的值 + x
                    // 返回的是 value 修改之前的值
                    return __sync_fetch_and_add(&value_, x);
                }

                T addAndGet(T x)
                {
                    // 返回的是修改之后的值
                    return getAndAdd(x) + x;
                }

                T incrementAndGet()
                {
                    // 自增1  返回自增之后的值
                    return addAndGet(1);
                }

                T decrementAndGet()
                {
                    // 自减1  返回自减之后的值
                    return addAndGet(-1);
                }

                void add(T x)
                {
                    // 自增x
                    getAndAdd(x);
                }

                void increment()
                {
                    // 自增 1
                    incrementAndGet();
                }

                void decrement()
                {
                    // 自减 1
                    decrementAndGet();
                }

                T getAndSet(T newValue)
                {
                    // 返回原来的值，并设置为新值
                    return __sync_lock_test_and_set(&value_, newValue);
                }

            private:
                // volatile 指出 value_ 是随时可能发生变化的，每次使用它的时候必须从 i的地址中读取
                volatile T value_;
        };
    }

    typedef detail::AtomicIntegerT<int32_t> AtomicInt32;
    typedef detail::AtomicIntegerT<int64_t> AtomicInt64;
}

#endif  // MUDUO_BASE_ATOMIC_H
