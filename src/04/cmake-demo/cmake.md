## 使用 cmake（centos 7）
 - 安装
    ```js
    // 切换到系统存放源代码的目录
    cd /usr/local/src
    curl -LO https://github.com/Kitware/CMake/releases/download/v3.22.2/cmake-3.22.2-linux-x86_64.tar.gz
    ```
 - 解压
    ```js
    tar -xvf cmake-3.22.2-linux-x86_64.tar.gz
    mv cmake-3.22.2-linux-x86_64 /usr/local/cmake
    ```
 - 添加到环境变量PATH中
    ```js
    echo 'export PATH="/usr/local/cmake/bin:$PATH"' >> ~/.bashrc
    source ~/.bashrc
    ```
 - 现在运行以下命令，应该能够看到所安装 CMake 的版本号
    ```js
    cmake --version
    ```
## 创建可执行文件 ---- 第一种方法
 - 创建 CMakeLists.txt 文件
 - 执行命令，之后会有Makefile文件
    ```js
    mkdir build
    cd build
    // CMakeLists.txt 文件在上一级目录
    cmake ..
    ```
 - make
    ```js
    make
    ```
 - 在`build/bin`目录下有可执行文件

## 创建可执行文件 ---- 第二种方法
 - 创建 build.sh文件
   ```js
    ./build.sh
   ```
 - 解决 `bash: ./build.sh: /bin/sh^M: 坏的解释器: 没有那个文件或目录`
    ```js
    sed -i 's/\r$//' build.sh
    ```
 - 删除
    ```js
    ./build.sh clean
    ```