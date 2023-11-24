# ares common build dependencies module

include_guard(GLOBAL)

option(ARES_SKIP_DEPS "Do not fetch prebuilt dependencies" OFF)
mark_as_advanced(ARES_SKIP_DEPS)

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
      if(found)
        set(skip TRUE)
      endif()
    endif()

    if(skip)
      message(STATUS "Setting up ${label} (${arch}) - skipped")
      continue()
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
      file(REMOVE_RECURSE "${dependencies_dir}/${destination}")
    endif()

    if(NOT EXISTS "${dependencies_dir}/${destination}")
      file(MAKE_DIRECTORY "${dependencies_dir}/${destination}")
      file(ARCHIVE_EXTRACT INPUT "${dependencies_dir}/${file}" DESTINATION "${dependencies_dir}")
    endif()

    file(WRITE "${dependencies_dir}/.dependency_${dependency}_${arch}.sha256" "${hash}")

    list(APPEND CMAKE_PREFIX_PATH "${dependencies_dir}/${destination}")
    message(STATUS "Setting up ${label} (${arch}) - done")
  endforeach()

  list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)

  set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} CACHE PATH "CMake prefix search path" FORCE)
endfunction()
