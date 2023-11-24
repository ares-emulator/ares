include_guard(GLOBAL)

include(helpers_common)

function(ares_configure_executable target)
  get_target_property(target_type ${target} TYPE)

  if(target_type STREQUAL EXECUTABLE)
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND
        "${CMAKE_COMMAND}" -E make_directory
        "${ARES_BUILD_OUTPUT_DIR}/${ARES_BUILD_EXECUTABLE_DESTINATION}/${CMAKE_INSTALL_BINDIR}"
      COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${target}>"
        "${ARES_BUILD_OUTPUT_DIR}/${ARES_BUILD_EXECUTABLE_DESTINATION}/${CMAKE_INSTALL_BINDIR}"
      COMMENT "Copy ${target} to binary directory"
      VERBATIM
    )

    install(TARGETS ${target} RUNTIME DESTINATION "${ARES_INSTALL_EXECUTABLE_DESTINATION}" COMPONENT ${target})
  endif()
endfunction()
