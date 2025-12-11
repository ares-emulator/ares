add_library(hiro STATIC)
add_library(ares::hiro ALIAS hiro)

target_link_libraries(hiro PRIVATE ares::nall::headers)

include(cmake/sources.cmake)

if(OS_WINDOWS)
  include(cmake/os-windows.cmake)
elseif(OS_MACOS)
  include(cmake/os-macos.cmake)
elseif(OS_LINUX OR OS_FREEBSD OR OS_OPENBSD)
  include(cmake/os-linux.cmake)
endif()

add_sourcery_command(hiro resource)

target_include_directories(hiro PRIVATE ${CMAKE_SOURCE_DIR})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${hiro_SOURCES})
