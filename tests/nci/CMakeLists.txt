add_executable(nci-test-client nci-test-client.cpp)

target_include_directories(nci-test-client PRIVATE ${CMAKE_SOURCE_DIR})

set_target_properties(nci-test-client PROPERTIES FOLDER tests PREFIX "")
target_enable_subproject(nci-test-client "NCI UDP server test harness")

target_link_libraries(nci-test-client PRIVATE ares::nall)
set(CONSOLE TRUE)
ares_configure_executable(nci-test-client)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES nci-test-client.cpp)
