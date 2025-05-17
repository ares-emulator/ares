target_sources(desktop-ui PRIVATE cmake/os-macos.cmake)

set_target_xcode_properties(
  desktop-ui
  PROPERTIES PRODUCT_BUNDLE_IDENTIFIER dev.ares.ares
             PRODUCT_NAME ares
             ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
             MARKETING_VERSION ${ARES_VERSION}
             GENERATE_INFOPLIST_FILE YES
             COPY_PHASE_STRIP NO
             CLANG_ENABLE_OBJC_ARC YES
             SKIP_INSTALL NO
             INSTALL_PATH "$(LOCAL_APPS_DIR)"
)

set_target_properties(
  desktop-ui
  PROPERTIES
    OUTPUT_NAME ares
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/ares.plist"
    XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY YES
    XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY YES
    XCODE_EMBED_PLUGINS_REMOVE_HEADERS_ON_COPY YES
    XCODE_EMBED_PLUGINS_CODE_SIGN_ON_COPY YES
)

target_add_resource(desktop-ui "${CMAKE_SOURCE_DIR}/ruby/video/metal/Shaders.metal" "Shaders")
if(ACTOOL_PROGRAM)
  target_add_resource(desktop-ui "${CMAKE_CURRENT_SOURCE_DIR}/resource/Assets.xcassets")
else()
  target_add_resource(desktop-ui "${CMAKE_CURRENT_SOURCE_DIR}/resource/AppIcon.icns")
endif()

function(target_install_shaders target)
  message(DEBUG "Installing shaders for target ${target}...")
  if(EXISTS "${slang_shaders_LOCATION}")
    file(GLOB_RECURSE data_files "${slang_shaders_LOCATION}/*")
    foreach(data_file IN LISTS data_files)
      cmake_path(RELATIVE_PATH data_file BASE_DIRECTORY "${slang_shaders_LOCATION}" OUTPUT_VARIABLE relative_path)
      cmake_path(GET relative_path PARENT_PATH relative_path)
      target_sources(${target} PRIVATE "${data_file}")
      set_property(SOURCE "${data_file}" PROPERTY MACOSX_PACKAGE_LOCATION "Resources/Shaders/${relative_path}")
      source_group("Resources/Shaders/${relative_path}" FILES "${data_file}")
    endforeach()
  endif()
endfunction()

function(target_install_database target)
  message(DEBUG "Installing shaders for target ${target}...")
  if(EXISTS "${CMAKE_SOURCE_DIR}/mia/Database")
    file(GLOB_RECURSE data_files "${CMAKE_SOURCE_DIR}/mia/Database/*")
    foreach(data_file IN LISTS data_files)
      cmake_path(
        RELATIVE_PATH
        data_file
        BASE_DIRECTORY "${CMAKE_SOURCE_DIR}/mia/Database/"
        OUTPUT_VARIABLE relative_path
      )
      cmake_path(GET relative_path PARENT_PATH relative_path)
      target_sources(${target} PRIVATE "${data_file}")
      set_property(SOURCE "${data_file}" PROPERTY MACOSX_PACKAGE_LOCATION "Resources/Database/${relative_path}")
      source_group("Resources/Database/${relative_path}" FILES "${data_file}")
    endforeach()
  endif()
endfunction()

# Add slang-shaders as a post-build script so we don't have an exceedingly long "Copy Files" phase
if(ARES_ENABLE_LIBRASHADER)
  if(TARGET libretro::slang_shaders)
    get_target_property(_required_macos libretro::slang_shaders MACOS_VERSION_REQUIRED)
    if(_required_macos VERSION_LESS_EQUAL CMAKE_OSX_DEPLOYMENT_TARGET)
      add_custom_command(
        OUTPUT
          "${CMAKE_CURRENT_BINARY_DIR}/$<IF:$<BOOL:${XCODE}>,$<CONFIG>,>/ares.app/Contents/Resources/Shaders/bilinear.slangp"
        COMMAND ditto "${slang_shaders_LOCATION}" "$<TARGET_BUNDLE_CONTENT_DIR:desktop-ui>/Resources/Shaders/"
        WORKING_DIRECTORY "$<TARGET_BUNDLE_CONTENT_DIR:desktop-ui>"
        COMMENT "Copying slang shaders to app bundle"
      )
      add_custom_target(
        bundled_shaders
        DEPENDS
          "${CMAKE_CURRENT_BINARY_DIR}/$<IF:$<BOOL:${XCODE}>,$<CONFIG>,>/ares.app/Contents/Resources/Shaders/bilinear.slangp"
      )
      add_dependencies(desktop-ui bundled_shaders)
    endif()
    unset(_required_macos)
  endif()
endif()

# Can't use target_add_resource for this since we only want it to occur in debug configurations
add_custom_command(
  TARGET desktop-ui
  POST_BUILD
  COMMAND $<$<CONFIG:Debug>:ditto>
  ARGS
    "${CMAKE_SOURCE_DIR}/ruby/video/metal/Shaders.metallib" "$<TARGET_BUNDLE_CONTENT_DIR:desktop-ui>/Resources/Shaders/"
  WORKING_DIRECTORY "$<TARGET_BUNDLE_CONTENT_DIR:desktop-ui>"
  COMMENT "Copying debug .metallib to app bundle"
)

target_install_database(desktop-ui)
