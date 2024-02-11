add_library(zstd STATIC
	lib/zstd.h
	lib/zstd_errors.h
	lib/zdict.h
	lib/common/debug.c
	lib/common/entropy_common.c
	lib/common/error_private.c
	lib/common/fse_decompress.c
	lib/common/pool.c
	lib/common/threading.c
	lib/common/xxhash.c
	lib/common/zstd_common.c
	lib/decompress/huf_decompress.c
	lib/decompress/zstd_ddict.c
	lib/decompress/zstd_decompress_block.c
	lib/decompress/zstd_decompress.c
)


target_include_directories(zstd PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/lib")
target_include_directories(zstd INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib")
target_compile_definitions(zstd PRIVATE ZSTD_DISABLE_ASM)
set_target_properties(zstd PROPERTIES POSITION_INDEPENDENT_CODE ON)

