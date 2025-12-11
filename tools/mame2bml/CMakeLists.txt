add_executable(mame2bml mame2bml.cpp)

target_include_directories(mame2bml PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(mame2bml PRIVATE ares::nall)
set_target_properties(mame2bml PROPERTIES FOLDER tools PREFIX "")
target_enable_subproject(mame2bml "mame2bml (MAME manifest converter)")
set(CONSOLE TRUE)
ares_configure_executable(mame2bml)
