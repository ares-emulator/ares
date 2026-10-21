# Frontend-native loaders and mapping/configuration support, without a GUI toolkit.
add_executable(ares-electron-host host.cpp
  native/runtime/runtime.cpp
  native/runtime/input/input.cpp
  native/runtime/emulator/emulator.cpp
  native/runtime/settings/settings.cpp)
target_include_directories(ares-electron-host PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_definitions(ares-electron-host PRIVATE MIA_LIBRARY)
target_link_libraries(ares-electron-host PRIVATE ares::ares ares::mia ares::ruby ares::nall)
if(ARES_ENABLE_CHD)
  target_link_libraries(ares-electron-host PRIVATE chdr-static)
endif()
set_target_properties(ares-electron-host PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(APPLE)
  # nall's shared startup object also contains its optional main wrapper. This
  # host supplies a plain main; discard the unused wrapper even without LTO.
  target_link_options(ares-electron-host PRIVATE "LINKER:-dead_strip")
  # The native host is an unbundled executable, with explicit runtime libraries.
  # The repository's macOS defaults disable CMake RPATH generation globally.
  get_filename_component(sdl_framework_directory "${SDL_LIBRARY}" DIRECTORY)
  target_link_options(ares-electron-host PRIVATE "LINKER:-rpath,${sdl_framework_directory}")
endif()
add_custom_command(TARGET ares-electron-host POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/mia/Database" "$<TARGET_FILE_DIR:ares-electron-host>/Database"
  COMMENT "Copying media databases for the native host")
if(APPLE)
  # For an unbundled command-line executable, nall's CFBundle-based
  # Path::program() resolves to the parent of its bin directory.
  add_custom_command(TARGET ares-electron-host POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/mia/Database" "$<TARGET_FILE_DIR:ares-electron-host>/../Database")
endif()

# Explicit target: a real N64 CPU fixture, without desktop adapters or a ROM.
add_executable(ares-electron-cpu-fixture EXCLUDE_FROM_ALL tests/cpu-debug-fixture.cpp)
target_include_directories(ares-electron-cpu-fixture PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(ares-electron-cpu-fixture PRIVATE ares::ares ares::nall)
set_target_properties(ares-electron-cpu-fixture PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(APPLE)
  target_link_options(ares-electron-cpu-fixture PRIVATE "LINKER:-dead_strip")
endif()
