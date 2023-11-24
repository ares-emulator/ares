target_sources(desktop-ui PRIVATE resource/ares.rc resource/ares.Manifest)

set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT desktop-ui)

if(ARES_ENABLE_LIBRASHADER)
  if(TARGET libretro::slang_shaders)
    add_custom_command(
      OUTPUT "${ARES_EXECUTABLE_DESTINATION}/desktop-ui/rundir/Shaders/bilinear.slangp" POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${ARES_EXECUTABLE_DESTINATION}/desktop-ui/rundir/Shaders/"
      COMMAND
        "${CMAKE_COMMAND}" -E copy_directory "${slang_shaders_LOCATION}"
        "${ARES_EXECUTABLE_DESTINATION}/desktop-ui/rundir/Shaders/"
      WORKING_DIRECTORY "."
      COMMENT "Copying slang shaders to rundir"
    )
    add_custom_target(
      bundled_shaders
      DEPENDS "${ARES_EXECUTABLE_DESTINATION}/desktop-ui/rundir/Shaders/bilinear.slangp"
    )
    add_dependencies(desktop-ui bundled_shaders)
  endif()
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/mia/Database")
  add_custom_command(
    TARGET desktop-ui
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${ARES_EXECUTABLE_DESTINATION}/desktop-ui/rundir/Database/"
    COMMAND
      "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/mia/Database/"
      "${ARES_EXECUTABLE_DESTINATION}/desktop-ui/rundir/Database/"
    COMMENT "Copying mia database to rundir"
  )
endif()
