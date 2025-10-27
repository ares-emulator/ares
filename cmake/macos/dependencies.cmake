# ares CMake macOS build dependencies module

include_guard(GLOBAL)

include(dependencies_common)

# _check_dependencies_macos: Set up macOS slice for _check_dependencies
function(_check_dependencies_macos)
  set(arch universal)
  set(platform macos-${arch})

  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/deps.json" deps)

  set(dependencies_dir "${CMAKE_CURRENT_SOURCE_DIR}/.deps")
  set(prebuilt_filename "ares-deps-macos-ARCH.tar.xz")
  set(prebuilt_destination "ares-deps-macos-ARCH")
  set(dependencies_list prebuilt)
  if(ARES_DEBUG_DEPENDENCIES)
    set(source_filename "ares-deps-macos-ARCH-source.tar.xz")
    set(source_destination "ares-deps-macos-ARCH-source")
    list(APPEND dependencies_list source)
  endif()

  _check_dependencies()

  execute_process(
    COMMAND "xattr" -r -s -d com.apple.quarantine "${dependencies_dir}/${destination}"
    RESULT_VARIABLE result
    COMMAND_ERROR_IS_FATAL ANY
  )
endfunction()

if(NOT ARES_SKIP_DEPS)
  _check_dependencies_macos()
endif()
