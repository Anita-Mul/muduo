// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_SINGLETON_H
#define MUDUO_BASE_SINGLETON_H

#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <stdlib.h> // atexit

// 线程安全Singleton类
namespace muduo
{
    // T就代替了int、float等数据类型，具体数据类型到实例化的时候再确定
    template<typename T>
    class Singleton : boost::noncopyable
    {
        public:
            static T& instance()
            {
                // 函数使用初值为PTHREAD_ONCE_INIT的once_control变量保证init_routine()函数在本进程执行序列中仅执行一次
                pthread_once(&ponce_, &Singleton::init);
                return *value_;
            }

        private:
            Singleton();
            ~Singleton();

            static void init()
            {
                value_ = new T();
                // C 库函数 int atexit(void (*func)(void)) 当程序正常终止时，调用指定的函数 func
                ::atexit(destroy);
            }

            static void destroy()
            {
                // 这个对象的类型 T 必须是一个完全类型
                // typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
                delete value_;
            }

        private:
            static pthread_once_t ponce_;
            static T*             value_;
    };

    template<typename T>
    pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

    template<typename T>
    T* Singleton<T>::value_ = NULL;
}
#endif