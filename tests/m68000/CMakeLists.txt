add_executable(m68000 m68000.cpp)

target_include_directories(m68000 PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(m68000 PRIVATE ares::ares)

set_target_properties(m68000 PROPERTIES FOLDER tests PREFIX "")
target_enable_subproject(m68000 "m68000 processor test harness")
set(CONSOLE TRUE)
ares_configure_executable(m68000)
