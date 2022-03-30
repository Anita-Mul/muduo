// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_THREADLOCALSINGLETON_H
#define MUDUO_BASE_THREADLOCALSINGLETON_H

#include <boost/noncopyable.hpp>
#include <assert.h>
#include <pthread.h>

/*
    引用和指针
    int a = 996;
    int *p = &a; // p是指针【0x00121】, &在此是求地址运算
    int &r = a;  // r是引用, &在此起标识作用
                 // 引用 r，是 a 的一个别名，在内存中 r 和 a 占有同一个存储单元
*/
namespace muduo
{
    template<typename T>
    class ThreadLocalSingleton : boost::noncopyable
    {
        public:
            // 返回单例对象的引用
            static T& instance()
            {
                if (!t_value_)
                {
                    t_value_ = new T();
                    deleter_.set(t_value_);
                }

                return *t_value_;
            }
            
            // 返回对象的指针
            static T* pointer()
            {
                return t_value_;
            }

        private:
            static void destructor(void* obj)
            {
                assert(obj == t_value_);
                // typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
                delete t_value_;
                t_value_ = 0;
            }

            class Deleter
            {
                public:
                  Deleter()
                  {
                      pthread_key_create(&pkey_, &ThreadLocalSingleton::destructor);
                  }

                  ~Deleter()
                  {
                      pthread_key_delete(pkey_);
                  }

                  // 传的是 t_value_ 的指针
                  void set(T* newObj)
                  {
                      assert(pthread_getspecific(pkey_) == NULL);
                      pthread_setspecific(pkey_, newObj);
                  }

                  pthread_key_t pkey_;
            };
            
            // __ 表示这个指针每个线程都有一份
            static __thread T* t_value_;
            // 用来销毁 t_value_ 指针所指向的对象的
            static Deleter deleter_;
    };

    template<typename T>
    __thread T* ThreadLocalSingleton<T>::t_value_ = 0;

    template<typename T>
    typename ThreadLocalSingleton<T>::Deleter ThreadLocalSingleton<T>::deleter_;
}
#endif
