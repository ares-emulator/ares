include_guard(GLOBAL)

option(ARES_BUILD_OFFICIAL "Use official project versioning" OFF)

function(ares_populate_version_info)
  if(DEFINED ARES_VERSION_OVERRIDE)
    set(ARES_VERSION ${ARES_VERSION_OVERRIDE})
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(
      COMMAND git describe --always --tags --exclude=nightly --dirty=-modified --match "v[0-9]*"
      OUTPUT_VARIABLE ARES_VERSION
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
      RESULT_VARIABLE ares_version_result
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(ARES_VERSION MATCHES "^.*modified")
      set(modified YES)
    endif()
  else()
    file(READ cmake/common/.archive-version.json archive_version_json)
    string(JSON archive_version_describe GET ${archive_version_json} describe)
    string(JSON ares_git_ref GET ${archive_version_json} ref-name)
    string(JSON ares_git_shorthash GET ${archive_version_json} hash)
    string(JSON ares_git_commit_date GET ${archive_version_json} commit-date)
    if(archive_version_describe)
      set(ARES_VERSION "${archive_version_describe}")
    else()
      set(ARES_VERSION "${ares_git_shorthash}")
    endif()
    string(STRIP "${ARES_VERSION}" ARES_VERSION)
  endif()

  string(REGEX REPLACE "^v([0-9]+(\\.[0-9]+)*)(-.*)?$" "\\1" ares_version_stripped "${ARES_VERSION}")
  string(REPLACE "." ";" ares_version_parts "${ares_version_stripped}")
  list(LENGTH ares_version_parts ares_version_parts_length)
  if(ares_version_parts_length GREATER 0)
    list(GET ares_version_parts 0 minor)
  else()
    set(minor 0)
  endif()
  if(ares_version_parts_length GREATER 1)
    list(GET ares_version_parts 1 patch)
  else()
    set(patch 0)
  endif()
  set(ARES_VERSION_CANONICAL "0.${minor}.${patch}")
  message(DEBUG "Canonical version set to ${ARES_VERSION_CANONICAL}")

  set(
    ares_version_not_found_string
    "Could not derive a canonical version string from available version information ('${ARES_VERSION}')."
  )

  if(NOT ARES_VERSION_CANONICAL MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    if(ARES_BUILD_OFFICIAL)
      message(FATAL_ERROR "${ares_version_not_found_string}")
    else()
      message(STATUS "${ares_version_not_found_string} - using fallback version 0.0.0")
    endif()
    set(ARES_VERSION_CANONICAL 0.0.0)
  endif()

  if(NOT ARES_BUILD_OFFICIAL)
    # Disambiguate unofficial builds
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
      execute_process(
        COMMAND git rev-parse --short HEAD
        OUTPUT_VARIABLE ares_git_shorthash
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      execute_process(
        COMMAND git symbolic-ref -q --short HEAD
        OUTPUT_VARIABLE ares_git_ref
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      execute_process(
        COMMAND git show -s --format=%cs
        OUTPUT_VARIABLE ares_git_commit_date
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      if(modified)
        set(ares_git_shorthash "${ares_git_shorthash}-modified")
      endif()
      if(NOT ares_git_ref)
        set(ARES_VERSION "${ares_git_shorthash} - ${ares_git_commit_date}")
      else()
        set(ARES_VERSION "${ares_git_ref} - (${ares_git_shorthash} - ${ares_git_commit_date})")
      endif()
    endif()
  endif()

  return(PROPAGATE ARES_VERSION ARES_VERSION_CANONICAL)
endfunction()

ares_populate_version_info()
