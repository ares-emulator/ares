# NOTE: This file should be restructured similarly to the other tests if the i8080 is ever used in an ares core.

add_executable(i8080 i8080.cpp ../../ares/component/processor/i8080/i8080.cpp)

target_include_directories(i8080 PRIVATE ${CMAKE_SOURCE_DIR})

set_target_properties(i8080 PROPERTIES FOLDER tests PREFIX "")
target_enable_subproject(i8080 "i8080 processor test harness")

target_link_libraries(i8080 PRIVATE ares::ares ares::nall)
set(CONSOLE TRUE)
ares_configure_executable(i8080)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES i8080.cpp)
