add_executable(genius genius.cpp)

target_sources(genius PRIVATE genius.hpp)

target_enable_subproject(genius "genius (database editor)")

target_include_directories(genius PRIVATE ${CMAKE_SOURCE_DIR})

if(OS_WINDOWS)
  include(cmake/os-windows.cmake)
elseif(OS_MACOS)
  include(cmake/os-macos.cmake)
elseif(OS_LINUX OR OS_FREEBSD OR OS_OPENBSD)
  include(cmake/os-linux.cmake)
endif()

target_link_libraries(genius PRIVATE ares::nall ares::nall::headers ares::hiro)

ares_configure_executable(genius)
set_target_properties(genius PROPERTIES FOLDER tools PREFIX "")
