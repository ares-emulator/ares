add_library(hiro STATIC)
add_library(ares::hiro ALIAS hiro)

target_link_libraries(hiro PUBLIC ares::nall)

include(cmake/sources.cmake)

if(OS_WINDOWS)
  include(cmake/os-windows.cmake)
elseif(OS_MACOS)
  include(cmake/os-macos.cmake)
elseif(OS_LINUX)
  include(cmake/os-linux.cmake)
elseif(OS_FREEBSD OR OS_OPENBSD)
  include(cmake/os-freebsd.cmake)
endif()

add_sourcery_command(hiro resource)

target_include_directories(hiro PRIVATE ${CMAKE_SOURCE_DIR})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${hiro_SOURCES})
