cmake_minimum_required(VERSION 3.10.0)
project(me VERSION 0.1.0 LANGUAGES C CXX)

# 头文件查找路径：让testLog.cpp可以 #include "LogStream.h"
include_directories(${PROJECT_SOURCE_DIR})

# 进入base/test子目录，读取里面的CMakeLists.txt
add_subdirectory(base/test)
