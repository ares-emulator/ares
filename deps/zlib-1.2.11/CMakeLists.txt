add_library(zlib STATIC
  zconf.h
  zlib.h
  adler32.c
  compress.c
  crc32.c
  crc32.h
  deflate.c
  deflate.h
  gzguts.h
  infback.c
  inffast.c
  inffast.h
  inffixed.h
  inflate.c
  inflate.h
  inftrees.c
  inftrees.h
  trees.c
  trees.h
  uncompr.c
  zutil.c
  zutil.h
)

target_include_directories(zlib PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(zlib INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(zlib PROPERTIES POSITION_INDEPENDENT_CODE ON)
