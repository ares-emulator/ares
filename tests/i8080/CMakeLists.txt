add_executable(i8080 i8080.cpp ../../ares/component/processor/i8080/i8080.cpp)

target_include_directories(i8080 PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(i8080 PRIVATE ares::ares)
set_target_properties(i8080 PROPERTIES FOLDER tests PREFIX "")
target_enable_subproject(i8080 "i8080 processor test harness")
ares_configure_executable(i8080)
