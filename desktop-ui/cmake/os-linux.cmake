option(ARES_BUNDLE_SHADERS "Add slang-shaders to the ares resources folder" ON)
mark_as_advanced(ARES_BUNDLE_SHADERS)

# Stage and install slang shaders
if(ARES_ENABLE_LIBRASHADER)
  if(TARGET libretro::slang_shaders)
    add_custom_command(
      OUTPUT "${ARES_BUILD_OUTPUT_DIR}/${ARES_INSTALL_DATA_DESTINATION}/Shaders/bilinear.slangp" POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${ARES_BUILD_OUTPUT_DIR}/${ARES_INSTALL_DATA_DESTINATION}/Shaders"
      COMMAND cp -R "${slang_shaders_LOCATION}/." "${ARES_BUILD_OUTPUT_DIR}/${ARES_INSTALL_DATA_DESTINATION}/Shaders"
      COMMENT "Copying slang shaders to staging directory"
    )
    add_custom_target(
      bundled_shaders
      DEPENDS "${ARES_BUILD_OUTPUT_DIR}/${ARES_INSTALL_DATA_DESTINATION}/Shaders/bilinear.slangp"
    )
    add_dependencies(desktop-ui bundled_shaders)
    if(ARES_BUNDLE_SHADERS)
      install(
        DIRECTORY "${slang_shaders_LOCATION}"
        DESTINATION "${ARES_INSTALL_DATA_DESTINATION}/Shaders"
        USE_SOURCE_PERMISSIONS
        COMPONENT desktop-ui
      )
    endif()
  endif()
endif()

# Stage and install mia database
add_custom_command(
  TARGET desktop-ui
  POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${ARES_BUILD_OUTPUT_DIR}/${ARES_INSTALL_DATA_DESTINATION}/Database"
  COMMAND
    cp -R "${CMAKE_SOURCE_DIR}/mia/Database/." "${ARES_BUILD_OUTPUT_DIR}/${ARES_INSTALL_DATA_DESTINATION}/Database/"
  COMMENT "Copying mia database to staging directory"
)

install(
  DIRECTORY "${CMAKE_SOURCE_DIR}/mia/Database/"
  DESTINATION "${ARES_INSTALL_DATA_DESTINATION}/Database"
  USE_SOURCE_PERMISSIONS
  COMPONENT desktop-ui
)

# Stage and install icon, .desktop file
add_custom_command(
  TARGET desktop-ui
  POST_BUILD
  COMMAND
    cp "${CMAKE_CURRENT_SOURCE_DIR}/resource/ares.desktop"
    "${ARES_BUILD_OUTPUT_DIR}/${ARES_INSTALL_DATA_DESTINATION}/ares.desktop"
  COMMAND
    cp "${CMAKE_CURRENT_SOURCE_DIR}/resource/ares.png"
    "${ARES_BUILD_OUTPUT_DIR}/${ARES_INSTALL_DATA_DESTINATION}/ares.png"
  COMMENT "Copying icon to staging directory"
)

install(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/resource/ares.desktop"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications"
  COMPONENT desktop-ui
)

install(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/resource/ares.png"
  DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/256x256/apps"
  COMPONENT desktop-ui
)
