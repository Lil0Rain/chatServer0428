cmake_minimum_required(VERSION 3.0)
project(chat)

# 生成 compile_commands.json 供 clangd/VSCode 使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 静态链接 C++ 运行时库，保留 glibc 动态（保证 DNS 解析等功能正常）
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")

#配置最终可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#配置头文件的搜索路径
include_directories (${PROJECT_SOURCE_DIR}/include)
include_directories (${PROJECT_SOURCE_DIR}/include/server)
include_directories (${PROJECT_SOURCE_DIR}/include/server/model)
include_directories (${PROJECT_SOURCE_DIR}/include/db)
include_directories (${PROJECT_SOURCE_DIR}/include/server/redis)
include_directories (${PROJECT_SOURCE_DIR}/include/ConnectionPool)

include_directories (${PROJECT_SOURCE_DIR}/thirdparty)
include_directories (/usr/include/mysql) 

#指定要搜索的子目录
add_subdirectory(src)
add_subdirectory(bench)
add_subdirectory(src/qt-client)
