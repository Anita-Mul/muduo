## muduo 介绍
 - 基于对象编程风格
 - non-trivial constructor 有用的构造函数
 - library framework
    库中类相对比较独立，我们编写应用的时候需要编写一些“胶水代码”来粘合。
    框架是能够应用于特定应用领域的，不需要编写过多的“胶水代码”来粘合
    框架提供用户注册一些回调函数，使得框架能够调用我们所编写的回调函数，这就使得控制反转了。
## muduo 安装
 - 切换到 root 模式下安装
 - github 下载 muduo 源码【Download ZIP】
    ```
    https://github.com/chenshuo/muduo
    ```
 - 将下载好的zip文件移动到共享文件夹，再从共享文件夹移动到`home/zoey/myLibrary/muduo`文件夹下
 - 解压 zip 文件
    ```
    unzip muduo-master.zip
    ```
## 安装 cmake
 - 之前安装过

## 安装 boost 库
 - 查看 boost 库的版本号
    ```
    // 1.53.0
    rpm -qa boost*
    ```
 - 官网下载 boost 1.53.0 的后缀为 zip 的压缩包【boost_1_53_0.zip】
    ```
    https://sourceforge.net/projects/boost/files/boost/
    ```
 - 同理，下载完之后移动到`home/zoey/myLibrary/boost`文件夹下
 - 解压该压缩包
    ```
    unzip boost_1_53_0.zip
    ```
 - 安装相关项
    ```
    yum install gcc gcc-c++ bzip2 bzip2-devel bzip2-libs python-devel
    ```
 - 进入文件夹
    ```
    cd boost_1_53_0
    ```
 - 执行里面的脚本
    ```
    ./bootstrap.sh
    ```
 - 执行之后发现多了可执行文件 b2
    ```
    sudo ./b2 install
    ```
## 安装非必须依赖库
```
yum install openssl
yum install protobuf
```
## 编译 muduo
 - `./build.sh -j2`
 - 当出现100%时，muduo库安装完毕

## source insight4.0 破解版下载及使用方法
 - 按照这个教程
    ```
    https://blog.csdn.net/ych9527/article/details/114324451
    ```
###### 有几点要说明
 - 第一步是下载中间的那个setup程序
 - 第二步是将下载好setup程序的那个文件夹打开，删除掉里面的sourceinsight4.exe文件
 - 然后再将压缩包里的sourceinsight4.exe文件移动到setup程序的那个文件夹
 - 第四步人家让选择一个.lic文件，你就选压缩包里的
