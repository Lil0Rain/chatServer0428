aux_source_directory(. BENCH_SRC)
add_executable(ConnectionPoolBench ${BENCH_SRC})
target_include_directories(ConnectionPoolBench PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/ConnectionPool
)
target_link_libraries(ConnectionPoolBench ConnectionPool mysqlclient pthread)
