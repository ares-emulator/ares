add_library(nall OBJECT main.cpp nall.cpp sljitAllocator.cpp)
add_library(ares::nall ALIAS nall)

include(cmake/sources.cmake)

find_package(Threads REQUIRED)
target_link_libraries(nall PRIVATE Threads::Threads)

target_link_libraries(nall PUBLIC sljit)
if(ARES_ENABLE_CHD)
  target_link_libraries(nall PUBLIC chdr-static)
  target_compile_definitions(nall PUBLIC ARES_ENABLE_CHD)
endif()
target_compile_definitions(nall PUBLIC CMAKE)
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
  target_sources(nall PRIVATE cmake/headers.cpp)
endif()

get_target_property(nall_SOURCES nall SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${nall_SOURCES})
set_source_files_properties(${nall_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(nall nall.cpp main.cpp sljitAllocator.cpp PROPERTIES HEADER_FILE_ONLY FALSE)

if(ARES_TREAT_NALL_AS_SYSTEM)
  target_include_directories(nall SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")
  target_include_directories(nall PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
  set_source_files_properties(nall cmake/headers.cpp PROPERTIES HEADER_FILE_ONLY FALSE)
else()
  target_include_directories(nall PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
endif()

# global compiler flags
target_compile_features(nall PUBLIC c_std_11)
set_target_properties(nall PROPERTIES C_EXTENSIONS OFF)

target_compile_features(nall PUBLIC cxx_std_17)
set_target_properties(nall PROPERTIES CXX_EXTENSIONS OFF)

target_compile_definitions(
  nall
  PUBLIC
    $<$<CONFIG:Debug>:BUILD_DEBUG>
    $<$<CONFIG:Release>:BUILD_RELEASE>
    $<$<CONFIG:RelWithDebInfo>:BUILD_RELEASE>
    $<$<CONFIG:MinSizeRel>:BUILD_MINIFIED>
)

if(ARES_BUILD_LOCAL)
  target_compile_definitions(nall PUBLIC BUILD_LOCAL)
endif()
