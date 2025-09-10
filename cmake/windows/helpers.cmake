include_guard(GLOBAL)

include(helpers_common)

# ares_configure_executable: Bundle entitlements, dependencies, resources to prepare macOS app bundle
function(ares_configure_executable target)
  if(CONSOLE)
    target_compile_definitions(${target} PRIVATE SUBSYTEM_CONSOLE)
    set_target_properties(${target} PROPERTIES WIN32_EXECUTABLE FALSE)
  else()
    target_compile_definitions(${target} PRIVATE SUBSYTEM_WINDOWS)
    set_target_properties(${target} PROPERTIES WIN32_EXECUTABLE TRUE)
  endif()
  set_target_properties(
    ${target}
    PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY
        "$<$<CONFIG:Debug,RelWithDebInfo,Release,MinSizeRel>:${ARES_EXECUTABLE_DESTINATION}/${target}/rundir>"
  )
  _bundle_dependencies(${target})
  install(TARGETS ${target} DESTINATION "${ARES_EXECUTABLE_DESTINATION}/${target}/rundir" COMPONENT Application)
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E echo "Copy binary ${target} to rundir"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${ARES_EXECUTABLE_DESTINATION}/${target}/rundir"
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${target}>" "${ARES_EXECUTABLE_DESTINATION}/${target}/rundir"
    COMMENT ""
    VERBATIM
  )
endfunction()

# _bundle_dependencies: Resolve third party dependencies and add them to Windows binary directory
function(_bundle_dependencies target)
  message(DEBUG "Discover dependencies of target ${target}...")
  set(found_dependencies)
  find_dependencies(TARGET ${target} DEPS_LIST found_dependencies)

  list(REMOVE_DUPLICATES found_dependencies)
  set(library_paths)

  foreach(library IN LISTS found_dependencies)
    get_target_property(library_type ${library} TYPE)
    get_target_property(is_imported ${library} IMPORTED)

    if(is_imported)
      get_target_property(imported_location ${library} IMPORTED_LOCATION)
      _check_library_location(${imported_location})
    endif()
  endforeach()

  if(NOT library_paths)
    return()
  endif()

  # Somewhat cursed, but in keeping with other platforms, make the build process create a runnable application.
  # That means copying dependencies and packaging as part of the build process. `cmake --install` will redundantly
  # perform this same process to conform with CMake convention.
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E echo "Copy dependencies to binary directory (${ARES_EXECUTABLE_DESTINATION})..."
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${ARES_OUTPUT_DIR}/${target}/rundir"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${library_paths}" "${ARES_EXECUTABLE_DESTINATION}/${target}/rundir"
    COMMENT "Copying dynamic dependencies to rundir"
    VERBATIM
    COMMAND_EXPAND_LISTS
  )

  install(
    FILES ${library_paths}
    DESTINATION "${ARES_EXECUTABLE_DESTINATION}"
    DESTINATION "${ARES_EXECUTABLE_DESTINATION}/${target}/rundir"
    COMPONENT Runtime
  )
endfunction()

# _check_library_location: Check for corresponding DLL given an import library path
macro(_check_library_location location)
  if(library_type STREQUAL "SHARED_LIBRARY")
    set(library_location "${location}")
  else()
    string(STRIP "${location}" location)
    if(location MATCHES ".+lib$")
      cmake_path(GET location FILENAME _dll_name)
      cmake_path(GET location PARENT_PATH _implib_path)
      string(REPLACE ".lib" ".dll" _dll_name "${_dll_name}")
      string(REPLACE ".dll" ".pdb" _pdb_name "${_dll_name}")

      find_program(_dll_path NAMES "${_dll_name}" HINTS ${_implib_path} NO_CACHE NO_DEFAULT_PATH)

      find_program(_pdb_path NAMES "${_pdb_name}" HINTS ${_implib_path} NO_CACHE NO_DEFAULT_PATH)

      if(_dll_path)
        set(library_location "${_dll_path}")
        set(library_pdb_location "${_pdb_path}")
      else()
        unset(library_location)
        unset(library_pdb_location)
      endif()
      unset(_dll_path)
      unset(_pdb_path)
      unset(_implib_path)
      unset(_dll_name)
      unset(_pdb_name)
    else()
      unset(library_location)
      unset(library_pdb_location)
    endif()
  endif()

  if(library_location)
    list(APPEND library_paths ${library_location})
  endif()
  if(library_pdb_location)
    list(APPEND library_paths ${library_pdb_location})
  endif()
  unset(location)
  unset(library_location)
  unset(library_pdb_location)
endmacro()
