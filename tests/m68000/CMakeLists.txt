add_executable(m68000 m68000.cpp)

target_include_directories(m68000 PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(m68000 PRIVATE ares::ares)

if(OS_WINDOWS)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(
      m68000
      PRIVATE nall
    )
  endif()
endif()

set_target_properties(m68000 PROPERTIES FOLDER tests PREFIX "")
target_enable_subproject(m68000 "m68000 processor test harness")
ares_configure_executable(m68000)
