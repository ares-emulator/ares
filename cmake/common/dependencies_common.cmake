# ares common build dependencies module

include_guard(GLOBAL)

option(ARES_SKIP_DEPS "Do not fetch prebuilt dependencies" OFF)
mark_as_advanced(ARES_SKIP_DEPS)

option(ARES_DEBUG_DEPENDENCIES "Fetch precompiled dependency source code and populate debugger initialization files for source view debugging" OFF)

# _check_dependencies: Fetch and extract pre-built ares build dependencies
function(_check_dependencies)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/deps.json" deps)

  string(JSON dependency_data GET ${deps} dependencies)

  foreach(dependency IN LISTS dependencies_list)
    string(JSON data GET ${dependency_data} ${dependency})
    string(JSON version GET ${data} version)
    string(JSON hash GET ${data} hashes ${platform})
    string(JSON url GET ${data} baseUrl)
    string(JSON label GET ${data} label)

    message(STATUS "Setting up ${label} (${arch})")

    set(file "${${dependency}_filename}")
    set(destination "${${dependency}_destination}")
    string(REPLACE "VERSION" "${version}" file "${file}")
    string(REPLACE "VERSION" "${version}" destination "${destination}")
    string(REPLACE "ARCH" "${arch}" file "${file}")
    string(REPLACE "ARCH" "${arch}" destination "${destination}")
    if(EXISTS "${dependencies_dir}/.dependency_${dependency}_${arch}.sha256")
      file(
        READ
        "${dependencies_dir}/.dependency_${dependency}_${arch}.sha256"
        ARES_DEPENDENCY_${dependency}_${arch}_HASH
      )
    endif()

    set(skip FALSE)
    if(ARES_DEPENDENCY_${dependency}_${arch}_HASH STREQUAL ${hash})
      if(EXISTS "${dependencies_dir}/${destination}")
        set(found TRUE)
      endif()
      if(found)
        set(skip TRUE)
      endif()
    endif()

    if(skip)
      message(STATUS "Setting up ${label} (${arch}) - skipped")
      list(APPEND CMAKE_PREFIX_PATH "${dependencies_dir}/${destination}")
      continue()
    else()
      file(REMOVE "${dependencies_dir}/${file}")
    endif()

    set(url ${url}/${version}/${file})

    if(NOT EXISTS "${dependencies_dir}/${file}")
      message(STATUS "Downloading ${url}")
      file(DOWNLOAD "${url}" "${dependencies_dir}/${file}" STATUS download_status EXPECTED_HASH SHA256=${hash})

      list(GET download_status 0 error_code)
      list(GET download_status 1 error_message)
      if(error_code GREATER 0)
        message(STATUS "Downloading ${url} - Failure")
        message(FATAL_ERROR "Unable to download ${url}, failed with error: ${error_message}")
        file(REMOVE "${dependencies_dir}/${file}")
      else()
        message(STATUS "Downloading ${url} - done")
      endif()
    endif()

    if(NOT ARES_DEPENDENCY_${dependency}_${arch}_HASH STREQUAL ${hash})
      if(EXISTS "${dependencies_dir}/${destination}")
        file(REMOVE_RECURSE "${dependencies_dir}/${destination}")
        message(STATUS "Removing outdated deps directory")
      endif()
    endif()

    if(NOT EXISTS "${dependencies_dir}/${destination}")
      file(MAKE_DIRECTORY "${dependencies_dir}/${destination}")
      message(STATUS "Unpacking ${label}")
      file(ARCHIVE_EXTRACT INPUT "${dependencies_dir}/${file}" DESTINATION "${dependencies_dir}")
      message(STATUS "Unpacking ${label} - done")
    endif()

    file(WRITE "${dependencies_dir}/.dependency_${dependency}_${arch}.sha256" "${hash}")

    list(APPEND CMAKE_PREFIX_PATH "${dependencies_dir}/${destination}")
    message(STATUS "Setting up ${label} (${arch}) - done")
  endforeach()

  list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)

  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} CACHE PATH "CMake prefix search path" FORCE)

  if(ARES_DEBUG_DEPENDENCIES)
    set(ARES_DEPS_SOURCE_MAPPINGS "")
    foreach(prefix IN LISTS CMAKE_PREFIX_PATH)
      if(EXISTS ${prefix}/buildhost.json)
        file(READ "${prefix}/buildhost.json" host_json_data)
        string(JSON host_working_directory GET ${host_json_data} build_host_working_directory)
        string(APPEND ARES_DEPS_SOURCE_MAPPINGS "${host_working_directory} ${prefix}/src ")
      endif()
    endforeach()
    if(NOT "${ARES_DEPS_SOURCE_MAPPINGS}" STREQUAL "")
      configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/resources/ares.lldbinit.in" .lldbinit @ONLY)
      configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/resources/ares.gdbinit.in" .gdbinit @ONLY)
    endif()
  endif()
endfunction()
