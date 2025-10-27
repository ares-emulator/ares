include_guard(GLOBAL)

include(dependencies_common)

# _check_dependencies_windows: Set up Windows slice for _check_dependencies
function(_check_dependencies_windows)
  set(dependencies_dir "${CMAKE_CURRENT_SOURCE_DIR}/.deps")
  set(prebuilt_filename "ares-deps-windows-ARCH.tar.xz")
  set(prebuilt_destination "ares-deps-windows-ARCH")
  set(dependencies_list prebuilt)
  if(ARES_DEBUG_DEPENDENCIES)
    set(source_filename "ares-deps-windows-ARCH-source.tar.xz")
    set(source_destination "ares-deps-windows-ARCH-source")
    list(APPEND dependencies_list source)
  endif()

  _check_dependencies()
endfunction()

if(NOT ARES_SKIP_DEPS)
  _check_dependencies_windows()
endif()
