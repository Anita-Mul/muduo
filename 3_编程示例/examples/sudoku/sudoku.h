#ifndef MUDUO_EXAMPLES_SUDOKU_SUDOKU_H
    #define MUDUO_EXAMPLES_SUDOKU_SUDOKU_H
    
    #include <muduo/base/Types.h>

    // FIXME, use (const char*, len) for saving memory copying.
    // 解出数独的函数
    muduo::string solveSudoku(const muduo::string& puzzle);
    const int kCells = 81;
#endif
