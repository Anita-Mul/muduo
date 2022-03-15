add_executable(fileutil_test FileUtil_test.cc)
target_link_libraries(fileutil_test muduo_base)

add_executable(logfile_test LogFile_test.cc)
target_link_libraries(logfile_test muduo_base)

add_executable(logging_test Logging_test.cc)
target_link_libraries(logging_test muduo_base)
