# ares CMake linux build dependencies module

include_guard(GLOBAL)

include(dependencies_common)

function(_check_dependencies_linux)
  # Detect architecture / Détecte l'architecture
  if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    set(arch arm64)
  else()
    set(arch x64)
  endif()
  set(platform linux-${arch})

  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/deps.json" deps)

  set(dependencies_dir "${CMAKE_CURRENT_SOURCE_DIR}/.deps")
  set(prebuilt_filename "ares-deps-linux-ARCH.tar.xz")
  set(prebuilt_destination "ares-deps-linux-ARCH")
  set(dependencies_list prebuilt)

  _check_dependencies()
endfunction()

if(NOT ARES_SKIP_DEPS)
  _check_dependencies_linux()
endif()
