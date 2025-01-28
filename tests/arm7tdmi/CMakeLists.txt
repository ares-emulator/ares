add_executable(arm7tdmi arm7tdmi.cpp)

target_include_directories(arm7tdmi PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(arm7tdmi PRIVATE ares::ares)

set_target_properties(arm7tdmi PROPERTIES FOLDER tests PREFIX "")
target_enable_subproject(arm7tdmi "arm7tdmi processor test harness")
set(CONSOLE TRUE)
ares_configure_executable(arm7tdmi)
