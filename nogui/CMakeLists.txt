add_executable(nogui nogui.cpp emulator.cpp)

target_include_directories(nogui PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_definitions(nogui PRIVATE MIA_LIBRARY=1)
target_link_libraries(nogui PRIVATE ares::ares ares::mia ares::nall)

if(ARES_ENABLE_CHD)
  target_link_libraries(nogui PRIVATE chdr-static)
endif()

set_target_properties(nogui PROPERTIES OUTPUT_NAME ares-nogui)

ares_configure_executable(nogui)
