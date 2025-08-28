add_library(ruby STATIC)
add_library(ares::ruby ALIAS ruby)

option(ARES_ENABLE_LIBRASHADER "Configure with librashader and slang shaders" ON)

if(OS_MACOS)
  target_sources(ruby PRIVATE ruby.mm)
else()
  target_sources(ruby PRIVATE ruby.cpp)
endif()

target_sources(ruby PRIVATE ruby.hpp)

target_sources(ruby PRIVATE audio/audio.cpp audio/audio.hpp audio/sdl.cpp)

target_sources(ruby PRIVATE input/input.cpp input/input.hpp input/sdl.cpp)

target_include_directories(ruby PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty)

target_sources(
  ruby
  PRIVATE
    video/video.cpp
    video/video.hpp
    video/opengl/bind.hpp
    video/opengl/main.hpp
    video/opengl/opengl.hpp
    video/opengl/surface.hpp
    video/opengl/texture.hpp
    video/opengl/utility.hpp
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

target_sources(ruby PRIVATE cmake/os-macos.cmake cmake/os-windows.cmake cmake/os-linux.cmake cmake/os-freebsd.cmake)

target_link_libraries(ruby PRIVATE ares::nall::headers)

target_include_directories(ruby PRIVATE ${CMAKE_SOURCE_DIR})

get_target_property(ruby_SOURCES ruby SOURCES)

set_source_files_properties(ruby ${ruby_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

if(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
  set_source_files_properties(ruby ruby.mm PROPERTIES HEADER_FILE_ONLY FALSE)
else()
  set_source_files_properties(ruby ruby.cpp PROPERTIES HEADER_FILE_ONLY FALSE)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${ruby_SOURCES})
