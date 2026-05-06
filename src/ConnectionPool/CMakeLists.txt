aux_source_directory(. POOL_SRC)
add_library(ConnectionPool ${POOL_SRC})
target_include_directories(ConnectionPool PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(ConnectionPool mysqlclient pthread)
