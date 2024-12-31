add_library(mia STATIC mia.cpp resource/resource.cpp)
add_library(ares::mia ALIAS mia)

target_sources(
  mia
  PRIVATE
    resource/resource.hpp
    medium/arcade.cpp
    medium/atari-2600.cpp
    medium/bs-memory.cpp
    medium/colecovision.cpp
    medium/famicom-disk-system.cpp
    medium/famicom.cpp
    medium/game-boy-advance.cpp
    medium/game-boy-color.cpp
    medium/game-boy.cpp
    medium/game-gear.cpp
    medium/mame.cpp
    medium/master-system.cpp
    medium/medium.cpp
    medium/medium.hpp
    medium/mega-32x.cpp
    medium/mega-cd.cpp
    medium/mega-drive.cpp
    medium/msx.cpp
    medium/msx2.cpp
    medium/myvision.cpp
    medium/neo-geo-crypt.hpp
    medium/neo-geo-pocket-color.cpp
    medium/neo-geo-pocket.cpp
    medium/neo-geo.cpp
    medium/nintendo-64.cpp
    medium/nintendo-64dd.cpp
    medium/pc-engine-cd.cpp
    medium/pc-engine.cpp
    medium/playstation.cpp
    medium/pocket-challenge-v2.cpp
    medium/saturn.cpp
    medium/sc-3000.cpp
    medium/sg-1000.cpp
    medium/sufami-turbo.cpp
    medium/super-famicom.cpp
    medium/supergrafx.cpp
    medium/wonderswan-color.cpp
    medium/wonderswan.cpp
    medium/zx-spectrum.cpp
)

target_sources(mia PRIVATE pak/pak.cpp pak/pak.hpp)

target_sources(
  mia
  PRIVATE
    program/game-importer.cpp
    program/game-importer.hpp
    program/game-manager.cpp
    program/game-manager.hpp
    program/home.cpp
    program/home.hpp
    program/program.cpp
    program/program.hpp
    program/system-selection.cpp
    program/system-selection.hpp
)

target_sources(mia PRIVATE settings/settings.cpp settings/settings.hpp)

target_sources(
  mia
  PRIVATE
    system/arcade.cpp
    system/atari-2600.cpp
    system/colecovision.cpp
    system/famicom.cpp
    system/game-boy-advance.cpp
    system/game-boy-color.cpp
    system/game-boy.cpp
    system/game-gear.cpp
    system/master-system.cpp
    system/mega-32x.cpp
    system/mega-cd-32x.cpp
    system/mega-cd.cpp
    system/mega-drive.cpp
    system/msx.cpp
    system/msx2.cpp
    system/myvision.cpp
    system/neo-geo-aes.cpp
    system/neo-geo-mvs.cpp
    system/neo-geo-pocket-color.cpp
    system/neo-geo-pocket.cpp
    system/nintendo-64.cpp
    system/nintendo-64dd.cpp
    system/pc-engine.cpp
    system/playstation.cpp
    system/pocket-challenge-v2.cpp
    system/saturn.cpp
    system/sc-3000.cpp
    system/sg-1000.cpp
    system/super-famicom.cpp
    system/supergrafx.cpp
    system/system.cpp
    system/system.hpp
    system/wonderswan-color.cpp
    system/wonderswan.cpp
    system/zx-spectrum-128.cpp
    system/zx-spectrum.cpp
)

target_compile_definitions(mia PRIVATE MIA_LIBRARY)

target_link_libraries(mia PUBLIC ares::nall ares::ares ares::hiro PRIVATE tzxfile)

if(ARES_BUILD_OPTIONAL_TARGETS)
  add_executable(mia-ui mia.cpp resource/resource.cpp)

  target_link_libraries(
    mia-ui
    PUBLIC ares::hiro ares::ares
    PRIVATE
      tzxfile
      "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,Carbon.framework>>"
      "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,IOKit.framework>>"
      "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,Security.framework>>"
  )

  if(WIN32)
    target_sources(mia-ui PRIVATE resource/mia.rc resource/mia.Manifest)
  endif()

  set_target_properties(
    mia-ui
    PROPERTIES
      OUTPUT_NAME mia
      MACOSX_BUNDLE TRUE
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resource/mia.plist"
      XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY YES
      XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY YES
      XCODE_EMBED_PLUGINS_REMOVE_HEADERS_ON_COPY YES
      XCODE_EMBED_PLUGINS_CODE_SIGN_ON_COPY YES
  )
  ares_configure_executable(mia-ui)
  target_enable_subproject(mia-ui "mia (manifest generator) standalone frontend")
  set_target_properties(mia-ui PROPERTIES FOLDER tools PREFIX "")
else()
  target_disable_subproject(mia-ui "mia (manifest generator) standalone frontend")
endif()

get_target_property(mia_SOURCES mia SOURCES)

set_source_files_properties(mia ${mia_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

set_source_files_properties(mia mia.cpp resource/resource.cpp PROPERTIES HEADER_FILE_ONLY FALSE)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${mia_SOURCES})
