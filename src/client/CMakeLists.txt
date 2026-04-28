#定义SRC_LIST，包含本目录下所有源文件
aux_source_directory(. SRC_LIST)

#指定生成可执行文件
add_executable(ChatClient ${SRC_LIST})
#指定可执行文件需要连接的库文件
target_link_libraries(ChatClient pthread) 