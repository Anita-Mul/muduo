// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCAL_H
#define MUDUO_BASE_THREADLOCAL_H

#include <boost/noncopyable.hpp>
#include <pthread.h>

/*
线程中特有的线程存储， Thread Specific Data 
大家都知道，在多线程程序中，所有线程共享程序中的变量。现在有一全局变量，所有线程都可以使用它，
改变它的值。而如果每个线程希望能单独拥有它，那么就需要使用线程存储了。表面上看起来这是一个全局变量，
所有线程都可以使用它，而它的值在每一个线程中又是单独存储的。这就是线程存储的意义。 
*/
namespace muduo
{
    template<typename T>
    class ThreadLocal : boost::noncopyable
    {
        public:
            ThreadLocal()
            {
                // 实际数据的销毁是由 ThreadLocal::destructor 来销毁的
                pthread_key_create(&pkey_, &ThreadLocal::destructor);
            }

            ~ThreadLocal()
            {
                // 仅仅只是销毁 key，并没有销毁实际的数据
                pthread_key_delete(pkey_);
            }

            T& value()
            {
                // 获取线程特定数据
                T* perThreadValue = static_cast<T*>(pthread_getspecific(pkey_));
                if (!perThreadValue) {
                    T* newObj = new T();
                    pthread_setspecific(pkey_, newObj);
                    perThreadValue = newObj;
                }
                return *perThreadValue;
            }

        private:
            static void destructor(void *x)
            {
                // 销毁
                T* obj = static_cast<T*>(x);
                // typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
                delete obj;
            }

        private:
            pthread_key_t pkey_;
    };
}
#endif
