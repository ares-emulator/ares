add_executable(chdr-benchmark benchmark.c)
target_link_libraries(chdr-benchmark PRIVATE chdr-static)
