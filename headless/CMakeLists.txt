add_executable(
  headless
  headless.cpp
  cli.cpp
  emulators.cpp
  parse.cpp
  runtime.cpp
  states.cpp
  settings.cpp
  video-output.cpp
)

target_include_directories(headless PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_definitions(headless PRIVATE MIA_LIBRARY=1)
target_link_libraries(headless PRIVATE ares::ares ares::mia ares::nall)

if(ARES_ENABLE_CHD)
  target_link_libraries(headless PRIVATE chdr-static)
endif()

set_target_properties(headless PROPERTIES OUTPUT_NAME ares-headless)

ares_configure_executable(headless)
