function(ares_register_verify target target_root)
  get_target_property(_srcs ${target} SOURCES)
  get_target_property(_target_srcdir ${target} SOURCE_DIR)

  # Create a file with all the declared sources for the target,
  # converted to absolute paths.
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/verify")
  set(_decl "${CMAKE_BINARY_DIR}/verify/${target}-declared.txt")
  file(WRITE "${_decl}" "")
  foreach(_s IN LISTS _srcs)
    get_filename_component(_abs "${_s}" REALPATH BASE_DIR "${_target_srcdir}")
    file(APPEND "${_decl}" "${_abs}\n")
  endforeach()

  # Run the verify_declared_sources.py script to check if all the sources
  # are declared in as target sources.
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    set(_verify_args "${CMAKE_SOURCE_DIR}/scripts/verify_declared_sources.py"
                     --build-dir "${CMAKE_BINARY_DIR}"
                     --target "${target}"
                     --target-root "${target_root}"
                     --declared "${_decl}"
                     --exclude "${CMAKE_SOURCE_DIR}/thirdparty"
                     --exclude "${CMAKE_SOURCE_DIR}/libco"
                     --exclude "${CMAKE_SOURCE_DIR}/tools"
                     --exclude "${CMAKE_SOURCE_DIR}/ares/n64/vulkan/parallel-rdp"
                     --exclude "${CMAKE_BINARY_DIR}")
    if(CMAKE_GENERATOR STREQUAL "Ninja")
      list(APPEND _verify_args --ninja "${CMAKE_MAKE_PROGRAM}")
    endif()
    add_custom_target(verify-${target}-run
      COMMAND ${Python3_EXECUTABLE} ${_verify_args}
      VERBATIM)
    add_dependencies(verify-${target}-run ${target})
  else()
    add_custom_target(verify-${target}-run
      COMMAND ${CMAKE_COMMAND} -E echo "verify-sources: Python3 not found; skipping ${target}."
      VERBATIM)
  endif()

  set_property(GLOBAL APPEND PROPERTY ARES_VERIFY_TARGETS "verify-${target}-run")
endfunction()

function(ares_define_verify_aggregate)
  get_property(_tgts GLOBAL PROPERTY ARES_VERIFY_TARGETS)
  add_custom_target(verify-sources)
  add_dependencies(verify-sources ${_tgts})
endfunction()


