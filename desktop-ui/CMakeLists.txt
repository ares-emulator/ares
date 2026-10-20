if(ARES_ENABLE_RCHEEVOS)
  set(retroachievements_source retroachievements/retroachievements.cpp)
else()
  set(retroachievements_source retroachievements/retroachievements-disabled.cpp)
endif()

add_executable(
  desktop-ui
  desktop-ui.cpp
  resource/resource.cpp
  input/input.cpp
  emulator/emulator.cpp
  game-browser/game-browser.cpp
  program/program.cpp
  presentation/presentation.cpp
  ${retroachievements_source}
  settings/settings.cpp
  tools/tools.cpp
)

target_sources(desktop-ui PRIVATE cmake/sources.cmake cmake/os-macos.cmake cmake/os-linux.cmake cmake/os-windows.cmake)

include(cmake/sources.cmake)

get_target_property(desktop-ui_SOURCES desktop-ui SOURCES)

set_source_files_properties(desktop-ui ${desktop-ui_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

set_source_files_properties(
  desktop-ui
  desktop-ui.cpp
  resource/resource.cpp
  input/input.cpp
  emulator/emulator.cpp
  game-browser/game-browser.cpp
  program/program.cpp
  presentation/presentation.cpp
  ${retroachievements_source}
  settings/settings.cpp
  tools/tools.cpp
  PROPERTIES HEADER_FILE_ONLY FALSE
)

if(ARES_ENABLE_LIBRASHADER)
  find_package(slang_shaders)
endif()

if(OS_WINDOWS)
  include(cmake/os-windows.cmake)
elseif(OS_MACOS)
  include(cmake/os-macos.cmake)
elseif(OS_LINUX OR OS_FREEBSD OR OS_OPENBSD)
  include(cmake/os-linux.cmake)
endif()

target_include_directories(desktop-ui PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(
  desktop-ui
  PRIVATE ares::ruby ares::hiro ares::ares ares::mia ares::nall
)

if(ARES_ENABLE_CHD)
  target_link_libraries(desktop-ui PRIVATE chdr-static)
endif()

if(ARES_ENABLE_RCHEEVOS)
  find_package(CURL CONFIG REQUIRED)
  find_package(rcheevos CONFIG REQUIRED)

  # The FDS adapter reads cartridge RAM through the Famicom core header.
  target_include_directories(desktop-ui PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty/ymfm/src)

  target_sources(
    desktop-ui
    PRIVATE
      retroachievements/platform/adapter.cpp
      retroachievements/platform/adapters/nintendo.cpp
      retroachievements/platform/adapters/sega.cpp
      retroachievements/platform/adapters/nec.cpp
      retroachievements/platform/adapters/sony.cpp
      retroachievements/platform/adapters/misc.cpp
      retroachievements/platform/hash_chd.cpp
  )
  if(OS_WINDOWS)
    target_sources(desktop-ui PRIVATE retroachievements/raintegration.cpp)
  endif()
  target_compile_definitions(desktop-ui PRIVATE ARES_ENABLE_RCHEEVOS)
  target_link_libraries(desktop-ui PRIVATE rcheevos::rcheevos CURL::libcurl)
  target_enable_feature(desktop-ui "RetroAchievements support")
endif()

set_target_properties(desktop-ui PROPERTIES OUTPUT_NAME ares)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${desktop-ui_SOURCES})

add_sourcery_command(desktop-ui resource)

ares_configure_executable(desktop-ui)
