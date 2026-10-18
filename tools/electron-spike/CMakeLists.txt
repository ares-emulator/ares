# Reuse the existing adapters and their supporting types. We do not construct
# Presentation/SettingsWindow or run Program's desktop event loop in this host.
add_executable(ares-electron-host host.cpp)
foreach(source IN ITEMS resource/resource.cpp input/input.cpp emulator/emulator.cpp
    game-browser/game-browser.cpp program/program.cpp presentation/presentation.cpp
    settings/settings.cpp tools/tools.cpp)
  target_sources(ares-electron-host PRIVATE "${CMAKE_SOURCE_DIR}/desktop-ui/${source}")
endforeach()
add_dependencies(ares-electron-host desktop-ui-resource)
target_include_directories(ares-electron-host PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_definitions(ares-electron-host PRIVATE ARES_SOURCE_ROOT="${CMAKE_SOURCE_DIR}/")
target_link_libraries(ares-electron-host PRIVATE ares::ares ares::mia ares::ruby ares::hiro ares::nall)
if(ARES_ENABLE_CHD)
  target_link_libraries(ares-electron-host PRIVATE chdr-static)
endif()
set_target_properties(ares-electron-host PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
if(APPLE)
  # Unlike desktop-ui this is not an app bundle with copied Frameworks.
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
