cmake_minimum_required(VERSION 3.11)

project(hello)

include_directories(./includes)

aux_source_directory(./tools TOOLS) #获得某个路径下的所有源文件，存放到变量TOOLS中

add_executable(cmake_learn_1 ./src/cmake_learn.cpp ${TOOLS})
