include_guard(GLOBAL)

set(_ares_version 0)

if(NOT DEFINED ARES_VERSION_OVERRIDE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  execute_process(
    COMMAND git describe --always --tags --exclude=nightly --dirty=-modified
    OUTPUT_VARIABLE ARES_VERSION
    ERROR_VARIABLE _git_describe_err
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    RESULT_VARIABLE _ares_version_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(_git_describe_err)
    message(FATAL_ERROR "Could not fetch ares version tag from git.\n" ${_git_describe_err})
  endif()
elseif(DEFINED ARES_VERSION_OVERRIDE)
  set(ARES_VERSION ${ARES_VERSION_OVERRIDE})
endif()

string(REGEX REPLACE "^v([0-9]+)(.*)$" "\\1.0.0\\2" _ares_version "${ARES_VERSION}")
string(REGEX REPLACE "(-[0-9]+-.*|-.+)" "" ARES_VERSION_CANONICAL "${_ares_version}")

unset(_ares_version)
