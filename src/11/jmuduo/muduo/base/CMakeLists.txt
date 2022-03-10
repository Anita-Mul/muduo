# 暂时只需要 Timestamp.cc 这个
# set(base_SRCS
#   Timestamp.cc
# )

# add_library(muduo_base ${base_SRCS})
# target_link_libraries(muduo_base pthread rt)

# install(TARGETS muduo_base DESTINATION lib)
# file(GLOB HEADERS "*.h")
# install(FILES ${HEADERS} DESTINATION include/muduo/base)

if(NOT CMAKE_BUILD_NO_EXAMPLES)
  add_subdirectory(tests)
endif()
