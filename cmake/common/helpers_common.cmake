# ares CMake common helper functions module

include_guard(GLOBAL)

function(add_sourcery_command target subdir)
  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/resource.cpp ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/resource.hpp
    COMMAND sourcery resource.bml resource.cpp resource.hpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/resource.bml
    VERBATIM
  )
  add_custom_target(
    ${target}-resource
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/resource.cpp ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/resource.hpp
  )
  add_dependencies(${target} ${target}-resource)
  set_target_properties(${target}-resource PROPERTIES FOLDER ${target} PREFIX "")
endfunction()

# message_configuration: Function to print configuration outcome
function(message_configuration)
  include(FeatureSummary)
  feature_summary(WHAT ALL VAR _feature_summary)

  message(DEBUG "${_feature_summary}")

  message(
    NOTICE
    "                          ..\n"
    "                           ::.\n"
    "                           .::.     ____ _________  _____\n"
    "                   ..     .:::.    / __ `/ ___/ _ \\/ ___/\n"
    "                    .-::::::::    / /_/ / /  /  __(__  )\n"
    "                      .::::.      \\__,_/_/   \\___/____/\n"
    "\n   ares version: ${ARES_VERSION} (${ARES_VERSION_CANONICAL})\n"
    "=================================================================================="
  )

  get_property(ARES_FEATURES_ENABLED GLOBAL PROPERTY ARES_FEATURES_ENABLED)
  list(SORT ARES_FEATURES_ENABLED COMPARE NATURAL CASE SENSITIVE ORDER ASCENDING)

  if(ARES_FEATURES_ENABLED)
    message(NOTICE "------------------------       Enabled Features           ------------------------")
    foreach(feature IN LISTS ARES_FEATURES_ENABLED)
      message(NOTICE " - ${feature}")
    endforeach()
  endif()

  get_property(ARES_FEATURES_DISABLED GLOBAL PROPERTY ARES_FEATURES_DISABLED)
  list(SORT ARES_FEATURES_DISABLED COMPARE NATURAL CASE SENSITIVE ORDER ASCENDING)

  if(ARES_FEATURES_DISABLED)
    message(NOTICE "------------------------       Disabled Features          ------------------------")
    foreach(feature IN LISTS ARES_FEATURES_DISABLED)
      message(NOTICE " - ${feature}")
    endforeach()
  endif()

  get_property(ARES_CORES_ENABLED GLOBAL PROPERTY ARES_CORES_ENABLED)
  list(SORT ARES_CORES_ENABLED COMPARE NATURAL CASE SENSITIVE ORDER ASCENDING)

  message(NOTICE "------------------------         Enabled Cores            ------------------------")
  foreach(core IN LISTS ARES_CORES_ENABLED)
    message(NOTICE " - ${core}")
  endforeach()

  get_property(ARES_CORES_DISABLED GLOBAL PROPERTY ARES_CORES_DISABLED)
  list(SORT ARES_CORES_DISABLED COMPARE NATURAL CASE SENSITIVE ORDER ASCENDING)

  message(NOTICE "------------------------         Disabled Cores           ------------------------")
  get_property(ARES_CORES_ALL GLOBAL PROPERTY ares_cores_all)
  list(APPEND ARES_CORES_DISABLED ${ARES_CORES_ALL})
  list(REMOVE_ITEM ARES_CORES_DISABLED ${ARES_CORES})
  foreach(core IN LISTS ARES_CORES_DISABLED)
    message(NOTICE " - ${core}")
  endforeach()

  # message(NOTICE "==================================================================================\n")

  get_property(ARES_SUBPROJECTS_ENABLED GLOBAL PROPERTY ARES_SUBPROJECTS_ENABLED)
  list(SORT ARES_SUBPROJECTS_ENABLED COMPARE NATURAL CASE SENSITIVE ORDER ASCENDING)

  if(ARES_SUBPROJECTS_ENABLED)
    message(NOTICE "-----------------------       Enabled Subprojects           ----------------------")
    foreach(subproject IN LISTS ARES_SUBPROJECTS_ENABLED)
      message(NOTICE " - ${subproject}")
    endforeach()
  endif()

  get_property(ARES_SUBPROJECTS_DISABLED GLOBAL PROPERTY ARES_SUBPROJECTS_DISABLED)
  list(SORT ARES_SUBPROJECTS_DISABLED COMPARE NATURAL CASE SENSITIVE ORDER ASCENDING)

  if(ARES_SUBPROJECTS_DISABLED)
    message(NOTICE "-----------------------       Disabled Subprojects          ----------------------")
    foreach(subproject IN LISTS ARES_SUBPROJECTS_DISABLED)
      message(NOTICE " - ${subproject}")
    endforeach()
  endif()
  message(NOTICE "==================================================================================")
endfunction()

function(target_enable_subproject target subproject_description)
  set_property(GLOBAL APPEND PROPERTY ARES_SUBPROJECTS_ENABLED "${subproject_description}")

  if(ARGN)
    target_compile_definitions(${target} PRIVATE ${ARGN})
  endif()
endfunction()

function(target_disable_subproject target subproject_description)
  set_property(GLOBAL APPEND PROPERTY ARES_SUBPROJECTS_DISABLED "${subproject_description}")

  if(ARGN)
    target_compile_definitions(${target} PRIVATE ${ARGN})
  endif()
endfunction()

# target_enable_feature: Adds feature to list of enabled application features and sets optional compile definitions
function(target_enable_feature target feature_description)
  set_property(GLOBAL APPEND PROPERTY ARES_FEATURES_ENABLED "${feature_description}")

  if(ARGN)
    target_compile_definitions(${target} PRIVATE ${ARGN})
  endif()
endfunction()

# enable_core: Add core to list of enabled cores
function(enable_core core_description)
  set_property(GLOBAL APPEND PROPERTY ARES_CORES_ENABLED "${core_description}")
endfunction()

# disable_core: Add core to list of disabled cores
function(disable_core core_description)
  set_property(GLOBAL APPEND PROPERTY ARES_CORES_DISABLED "${core_description}")
endfunction()

# target_disable_feature: Adds feature to list of disabled application features and sets optional compile definitions
function(target_disable_feature target feature_description)
  set_property(GLOBAL APPEND PROPERTY ARES_FEATURES_DISABLED "${feature_description}")

  if(ARGN)
    target_compile_definitions(${target} PRIVATE ${ARGN})
  endif()
endfunction()

# Find all direct and transitive dependencies of a target
function(find_dependencies)
  set(oneValueArgs TARGET DEPS_LIST)
  set(multiValueArgs)
  cmake_parse_arguments(arg_fd "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  get_target_property(linked_libraries ${arg_fd_TARGET} LINK_LIBRARIES)
  get_target_property(interface_link_libraries ${arg_fd_TARGET} INTERFACE_LINK_LIBRARIES)

  list(APPEND linked_libraries ${interface_link_libraries})
  list(REMOVE_DUPLICATES linked_libraries)

  foreach(library IN LISTS linked_libraries)
    set(found_target_name "")
    _extract_target_from_link_expression(
      LIBRARY
        ${library}
      TARGET_VAR
        found_target_name
    )
    if(found_target_name)
      if(NOT ${found_target_name} IN_LIST ${arg_fd_DEPS_LIST})
        list(APPEND ${arg_fd_DEPS_LIST} ${found_target_name})
        message(DEBUG "adding ${found_target_name}")
        find_dependencies(
          TARGET
            ${found_target_name}
          DEPS_LIST
           ${arg_fd_DEPS_LIST}
        )
        set(${arg_fd_DEPS_LIST} ${${arg_fd_DEPS_LIST}} PARENT_SCOPE)
      endif()
    endif()
  endforeach()
  set(${arg_fd_DEPS_LIST} ${${arg_fd_DEPS_LIST}} PARENT_SCOPE)
endfunction()

# Extract a CMake target name, if there is one that is applicable, from a target_link_libraries argument
function(_extract_target_from_link_expression)
  set(oneValueArgs LIBRARY TARGET_VAR)
  set(multiValueArgs)
  cmake_parse_arguments(arg_dt "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  message(DEBUG "arg target is ${arg_dt_LIBRARY}, evaluation is ${${arg_dt_LIBRARY}}")

  if(NOT library MATCHES ".+::.+")
    return()
  endif()

  if(TARGET ${library})
    set(${arg_dt_TARGET_VAR} ${library})
  elseif(arg_dt_LIBRARY MATCHES "\\$<LINK_LIBRARY:[^>]+>")
    # Generator expression specifying link method found. Consider parameter following the link method to be a CMake
    # target.
    string(REGEX REPLACE "\\$<LINK_LIBRARY:[^,]+,([^>]+)>" "\\1" gen_linkspec "${arg_dt_LIBRARY}")

    message(DEBUG "gen_linkspec is ${gen_linkspec}")

    if(TARGET ${gen_linkspec})
      message(DEBUG "setting arg_dt_TARGET_VAR to ${gen_linkspec}")
      set(${arg_dt_TARGET_VAR} ${gen_linkspec})
    endif()
  elseif(arg_dt_LIBRARY MATCHES "\\$<\\$<BOOL:[^>]+>:.+>")
    # Boolean generator expression found. Consider the parameter following the boolean a CMake target, and if true,
    # propagate it upward.
    string(REGEX REPLACE "\\$<\\$<BOOL:([^>]+)>:([^>]+)>" "\\1;\\2" gen_expression "${arg_dt_LIBRARY}")
    list(GET gen_expression 0 gen_boolean)
    list(GET gen_expression 1 gen_library)
    if(${gen_boolean})
      set(${arg_dt_TARGET_VAR} "${gen_library}")
    endif()
  elseif(
    arg_dt_LIBRARY MATCHES "\\$<\\$<PLATFORM_ID:[^>]+>:.+>"
    OR arg_dt_LIBRARY MATCHES "\\$<\\$<NOT:\\$<PLATFORM_ID:[^>]+>>:.+>"
  )
    # Platform-dependent generator expression found. Platforms are a comma-separated list of CMake host OS identifiers.
    # Convert to CMake list and check if current host OS is contained in list.
    string(REGEX REPLACE "\\$<.*\\$<PLATFORM_ID:([^>]+)>>?:([^>]+)>" "\\1;\\2" gen_expression "${arg_dt_LIBRARY}")
    list(GET gen_expression 0 gen_platform)
    list(GET gen_expression 1 gen_library)
    string(REPLACE "," ";" gen_platform "${gen_platform}")

    if(arg_dt_LIBRARY MATCHES "\\$<\\$<NOT:.+>.+>")
      if(NOT CMAKE_SYSTEM_NAME IN_LIST gen_platform)
        set(${arg_dt_TARGET_VAR} "${gen_library}")
      endif()
    else()
      if(CMAKE_SYSTEM_NAME IN_LIST gen_platform)
        set(${arg_dt_TARGET_VAR} "${gen_library}")
      endif()
    endif()
  elseif(arg_dt_LIBRARY MATCHES "\\$<LINK_ONLY:[^>]+>")
    # Non-specific target link expression found. Since no other expressions matched, consider the parameter following
    # LINK_ONLY to be a CMake target.
    string(REGEX REPLACE "\\$<LINK_ONLY:([^>]+)>" "\\1" gen_library "${arg_dt_LIBRARY}")

    message(DEBUG "gen_library is ${gen_library}")

    if(TARGET ${gen_library})
      message(DEBUG "setting arg_dt_TARGET_VAR to ${gen_library}")
      set(${arg_dt_TARGET_VAR} ${gen_library})
    endif()
  else()
    # Unknown or unimplemented generator expression found. Abort script run to either add to ignore list or implement
    # detection.
    message(FATAL_ERROR "${arg_dt_LIBRARY} is an unsupported generator expression for linked libraries.")
    set(${arg_dt_TARGET_VAR} "" PARENT_SCOPE)
  endif()
  return(PROPAGATE ${arg_dt_TARGET_VAR})
endfunction()
# endif()
