// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include <muduo/base/Exception.h>

#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>
#include <stdio.h>

using namespace muduo;

Exception::Exception(const char* msg)
  : message_(msg)
{
    fillStackTrace();
}

Exception::Exception(const string& msg)
  : message_(msg)
{
    fillStackTrace();
}

// throw (int, char, exception) 指明当前函数能够抛出的异常类型
// 如果函数不会抛出任何异常，那么( )中什么也不写
// ~Exception() 函数就不能抛出任何类型的异常了，即使抛出了，try 也检测不到
Exception::~Exception() throw ()
{
}

const char* Exception::what() const throw()
{
    // c_str() 返回当前字符串的首字符地址
    return message_.c_str();
}

const char* Exception::stackTrace() const throw()
{
    return stack_.c_str();
}


/**
 * backtrace 栈回溯，保存各个栈帧的地址
 * backtrace_symbols 根据地址，转换成相应的函数符号
 */
void Exception::fillStackTrace()
{
    const int len = 200;
    void* buffer[len];
    int nptrs = ::backtrace(buffer, len);
    // 指向的是一个指针数组，这个指针数组里的每个指针指向一个字符串
    char** strings = ::backtrace_symbols(buffer, nptrs);

    if (strings)
    {
        for (int i = 0; i < nptrs; ++i)
        {
            // TODO demangle funcion name with abi::__cxa_demangle
            // 可以直接这样写
            //stack_.append(strings[i]);
            // strings[i] 指向的是字符串指针
            stack_.append(demangle(strings[i]));
            stack_.push_back('\n');
        }

        free(strings);
    }
}


// 可以让错误输出的那个字符串更好看
string Exception::demangle(const char* symbol)
{
    size_t size;
    int status;
    char temp[128];
    char* demangled;

    // first, try to demangle a c++ name
    // abi::__cxa_demangle
    if (1 == sscanf(symbol, "%*[^(]%*[^_]%127[^)+]", temp)) {
        if (NULL != (demangled = abi::__cxa_demangle(temp, NULL, &size, &status))) {
            string result(demangled);
            free(demangled);
            return result;
        }
    }

    //if that didn't work, try to get a regular c symbol
    if (1 == sscanf(symbol, "%127s", temp)) {
        return temp;
    }
  
    //if all else fails, just return the symbol
    return symbol;
}
