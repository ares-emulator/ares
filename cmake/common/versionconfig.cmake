include_guard(GLOBAL)

function(populate_ares_version_info)
  if(DEFINED ARES_VERSION_OVERRIDE)
    set(ARES_VERSION ${ARES_VERSION_OVERRIDE})
  elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    # Get current branch or tag
    execute_process(
        COMMAND git symbolic-ref --short -q HEAD
        OUTPUT_VARIABLE GIT_REF
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    if(NOT GIT_REF)
        execute_process(
            COMMAND git describe --tags --exact-match
            OUTPUT_VARIABLE GIT_REF
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        )
    endif()

    # Get short hash
    execute_process(
        COMMAND git rev-parse --short HEAD
        OUTPUT_VARIABLE GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )

    # Check for local modifications
    execute_process(
        COMMAND git status --porcelain
        OUTPUT_VARIABLE GIT_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )
    if(GIT_STATUS)
        set(GIT_HASH "${GIT_HASH}-modified")
    endif()

    # Get commit date
    execute_process(
        COMMAND git show -s --format=%cd --date=short
        OUTPUT_VARIABLE GIT_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    )

    # Combine all parts
    set(ARES_VERSION "${GIT_REF} (${GIT_HASH} - ${GIT_DATE})")
  else()
    file(READ cmake/common/.archive-version ares_tarball_version)
    string(STRIP "${ares_tarball_version}" ares_tarball_version)
    set(ARES_VERSION "${ares_tarball_version}")
  endif()
  
  string(REGEX REPLACE "^v([0-9]+(\\.[0-9]+)*)-.*" "\\1" ares_version_stripped "${ARES_VERSION}")
  string(REPLACE "." ";" ares_version_parts "${ares_version_stripped}")
  list(LENGTH ares_version_parts ares_version_parts_length)
  list(GET ares_version_parts 0 major)
  if(ares_version_parts_length GREATER 1)
    list(GET ares_version_parts 1 minor)
  else()
    set(minor 0)
  endif()
  if(ares_version_parts_length GREATER 2)
    list(GET ares_version_parts 2 patch)
  else()
    set(patch 0)
  endif()
  set(ARES_VERSION_CANONICAL "${major}.${minor}.${patch}")
  message(DEBUG "Canonical version set to ${ARES_VERSION_CANONICAL}")
  return(PROPAGATE ARES_VERSION ARES_VERSION_CANONICAL)
endfunction()

populate_ares_version_info()

