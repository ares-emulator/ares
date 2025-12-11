add_library(nall OBJECT main.cpp nall.cpp sljitAllocator.cpp)
add_library(ares::nall ALIAS nall)

add_library(nall-headers INTERFACE)
add_library(ares::nall::headers ALIAS nall-headers)
target_link_libraries(nall PUBLIC ares::nall::headers)
if(ARES_ENABLE_CHD)
  target_compile_definitions(nall-headers INTERFACE ARES_ENABLE_CHD)
endif()
target_compile_definitions(
  nall-headers
  INTERFACE
    $<$<CONFIG:Debug>:BUILD_DEBUG>
    $<$<CONFIG:Release>:BUILD_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:BUILD_RELEASE>
    $<$<CONFIG:MinSizeRel>:BUILD_MINIFIED>
)

include(cmake/sources.cmake)

find_package(Threads REQUIRED)
target_link_libraries(nall PRIVATE Threads::Threads)

target_link_libraries(nall PUBLIC sljit)
if(ARES_ENABLE_CHD)
  target_link_libraries(nall PUBLIC chdr-static)
endif()
target_compile_options(
  nall
  PRIVATE
    $<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang,GNU>:-Wno-switch
    -Wno-parentheses
    -Wno-shorten-64-to-32
    -Wno-comma
    -Wno-sign-compare
    -Wno-deprecated-literal-operator
    -Wno-unused>
)

if(OS_WINDOWS)
  include(cmake/os-windows.cmake)
elseif(OS_MACOS)
  include(cmake/os-macos.cmake)
elseif(OS_LINUX)
  include(cmake/os-linux.cmake)
elseif(OS_FREEBSD OR OS_OPENBSD)
  include(cmake/os-freebsd.cmake)
endif()

option(ARES_TREAT_NALL_AS_SYSTEM "Enable system library semantics for nall in other targets" ON)
mark_as_advanced(ARES_TREAT_NALL_AS_SYSTEM)

if(ARES_TREAT_NALL_AS_SYSTEM)
  add_library(nall-static-compile-test STATIC cmake/headers.cpp)
  set_target_properties(nall-static-compile-test PROPERTIES FOLDER "generated" PREFIX "")
  target_include_directories(nall-static-compile-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()

get_target_property(nall_SOURCES nall SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${nall_SOURCES})
set_source_files_properties(${nall_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(nall nall.cpp main.cpp sljitAllocator.cpp PROPERTIES HEADER_FILE_ONLY FALSE)

if(ARES_TREAT_NALL_AS_SYSTEM)
  target_include_directories(nall-headers SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")
else()
  target_include_directories(nall-headers PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()

if(ARES_BUILD_LOCAL)
  target_compile_definitions(nall-headers INTERFACE BUILD_LOCAL)
endif()
