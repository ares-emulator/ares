function(ares_register_verify target target_root)
  if(NOT TARGET ${target})
    return()
  endif()

  get_target_property(_srcs ${target} SOURCES)
  if(NOT _srcs)
    set(_srcs)
  endif()

  # Resolve relative source paths against the target's own SOURCE_DIR
  get_target_property(_target_srcdir ${target} SOURCE_DIR)
  if(NOT _target_srcdir)
    set(_target_srcdir ${CMAKE_CURRENT_SOURCE_DIR})
  endif()

  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/verify")
  set(_decl "${CMAKE_BINARY_DIR}/verify/${target}-declared.txt")
  file(WRITE "${_decl}" "")
  foreach(_s IN LISTS _srcs)
    if(NOT IS_ABSOLUTE "${_s}")
      get_filename_component(_abs "${_s}" REALPATH BASE_DIR "${_target_srcdir}")
    else()
      set(_abs "${_s}")
    endif()
    file(APPEND "${_decl}" "${_abs}\n")
  endforeach()

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
  if(NOT _tgts)
    add_custom_target(verify-sources COMMAND ${CMAKE_COMMAND} -E echo "No verify targets.")
    return()
  endif()

  add_custom_target(verify-sources)
  add_dependencies(verify-sources ${_tgts})
endfunction()


