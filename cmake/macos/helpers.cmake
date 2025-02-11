include_guard(GLOBAL)

include(helpers_common)

# set_target_xcode_properties: Sets Xcode-specific target attributes
function(set_target_xcode_properties target)
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs PROPERTIES)
  cmake_parse_arguments(PARSE_ARGV 0 _STXP "${options}" "${oneValueArgs}" "${multiValueArgs}")

  message(DEBUG "Setting Xcode properties for target ${target}...")

  while(_STXP_PROPERTIES)
    list(POP_FRONT _STXP_PROPERTIES key value)
    set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_${key} "${value}")
  endwhile()
endfunction()

# ares_configure_executable: Bundle entitlements, dependencies, resources to prepare macOS app bundle
function(ares_configure_executable target)
  get_target_property(target_type ${target} TYPE)

  if(target_type STREQUAL EXECUTABLE)
    get_target_property(is_bundle ${target} MACOSX_BUNDLE)
    get_target_property(target_output_name ${target} OUTPUT_NAME)
    if(is_bundle)
      set(plist_file "resource/${target_output_name}.plist.in")
      configure_file(${plist_file} "${target_output_name}.plist" @ONLY)
      set(entitlements_file "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/entitlements.plist")
      if(NOT EXISTS "${entitlements_file}")
        message(AUTHOR_WARNING "Target ${target} is missing an entitlements file in its cmake directory.")
      else()
        set_target_properties(${target} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${entitlements_file}")
      endif()
    endif()

    _bundle_dependencies(${target})

    install(TARGETS ${target} BUNDLE DESTINATION "." COMPONENT Application)
  endif()
endfunction()

# target_add_resource: Helper function to add a specific resource to a bundle
function(target_add_resource target resource)
  message(DEBUG "Add resource ${resource} to target ${target} at destination ${destination}...")
  target_sources(${target} PRIVATE "${resource}")
  if(ARGN)
    set(subpath ${ARGN})
    set_property(SOURCE "${resource}" PROPERTY MACOSX_PACKAGE_LOCATION "Resources/${subpath}")
  else()
    if(UNUSED) # ${resource} MATCHES ".+\\.xcassets")
      # todo: asset archive compilation on non-Xcode; very annoying
      add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Assets.car ${CMAKE_CURRENT_BINARY_DIR}/AppIcon.icns
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/ac_generated_info.plist
        MAIN_DEPENDENCY ${resource}/Contents.json
        COMMAND
          actool --output-format human-readable-text --notices --warnings --target-device mac --platform macosx
          --minimum-deployment-target ${CMAKE_OSX_DEPLOYMENT_TARGET} --app-icon AppIcon --output-partial-info-plist
          ${CMAKE_CURRENT_BINARY_DIR}/ac_generated_info.plist --development-region en --enable-on-demand-resources NO
          --compile ${CMAKE_CURRENT_BINARY_DIR} ${resource}
      )
      target_sources(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/Assets.car ${CMAKE_CURRENT_BINARY_DIR}/AppIcon.icns)
      set_source_files_properties(
        ${CMAKE_CURRENT_BINARY_DIR}/Assets.car
        ${CMAKE_CURRENT_BINARY_DIR}/AppIcon.icns
        PROPERTIES HEADER_FILE_ONLY TRUE MACOSX_PACKAGE_LOCATION Resources
      )
    else()
      set_property(SOURCE "${resource}" PROPERTY MACOSX_PACKAGE_LOCATION Resources)
    endif()
  endif()
  source_group("Resources" FILES "${resource}")
endfunction()

# _bundle_dependencies: Resolve 3rd party dependencies and add them to macOS app bundle
function(_bundle_dependencies target)
  message(DEBUG "Discover dependencies of target ${target}...")
  set(found_dependencies)
  find_dependencies(TARGET ${target} DEPS_LIST found_dependencies)
  list(REMOVE_DUPLICATES found_dependencies)

  set(library_paths)
  set(bundled_targets)
  file(GLOB sdk_library_paths /Applications/Xcode*.app)
  set(system_library_path "/usr/lib/")

  foreach(library IN LISTS found_dependencies)
    get_target_property(library_type ${library} TYPE)
    get_target_property(is_framework ${library} FRAMEWORK)
    get_target_property(is_imported ${library} IMPORTED)

    if(is_imported)
      get_target_property(imported_location ${library} LOCATION)
      if(NOT imported_location)
        continue()
      endif()

      set(is_xcode_framework FALSE)
      set(is_system_framework FALSE)

      foreach(sdk_library_path IN LISTS sdk_library_paths)
        if(is_xcode_framework)
          break()
        endif()
        cmake_path(IS_PREFIX sdk_library_path "${imported_location}" is_xcode_framework)
      endforeach()
      cmake_path(IS_PREFIX system_library_path "${imported_location}" is_system_framework)

      unset(_required_macos)
      get_target_property(_required_macos ${library} MACOS_VERSION_REQUIRED)

      if(is_system_framework OR is_xcode_framework)
        continue()
      elseif(is_framework)
        file(REAL_PATH "../../.." library_location BASE_DIRECTORY "${imported_location}")
      elseif(_required_macos VERSION_GREATER CMAKE_OSX_DEPLOYMENT_TARGET)
        continue()
      elseif(NOT library_type STREQUAL "STATIC_LIBRARY")
        if(NOT imported_location MATCHES ".+\\.a")
          set(library_location "${imported_location}")
        else()
          continue()
        endif()
      else()
        continue()
      endif()

      if(XCODE AND ${target} STREQUAL mia-ui AND ${library} STREQUAL "MoltenVK::MoltenVK")
        message(DEBUG "Working around https://gitlab.kitware.com/cmake/cmake/-/issues/23675")
        continue()
      endif()

      list(APPEND bundled_targets ${library})

      list(APPEND library_paths ${library_location})
    elseif(NOT is_imported AND library_type STREQUAL "SHARED_LIBRARY")
      list(APPEND library_paths ${library})
    endif()
  endforeach()

  list(REMOVE_DUPLICATES library_paths)

  if(UNUSED)
    # One of these would be nice, but we cannot install IMPORTed targets (librashader, SDL, MoltenVK). We
    # could use install(FILES ...), but that wouldn't fixup rpaths, which defeats the purpose of using
    # install() in the first place.
    install(
      TARGETS ${target} ${bundled_targets}
      RUNTIME_DEPENDENCIES
      RUNTIME DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks"
      LIBRARY DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks"
      FRAMEWORK DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks"
      BUNDLE DESTINATION "."
    )
    install(
      IMPORTED_RUNTIME_ARTIFACTS ${target}
      RUNTIME DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks"
      LIBRARY DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks"
      FRAMEWORK DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks"
      BUNDLE DESTINATION "."
    )
  endif()

  if(XCODE)
    set_property(TARGET ${target} APPEND PROPERTY XCODE_EMBED_FRAMEWORKS ${library_paths})
  else()
    # This isn't strictly good practice, since the --install command should ideally be responsible for assembling
    # the bundle. Using install commands to assemble an app bundle with pre-built dependencies isn't very feasible
    # though, and seems to violate various CMake principles. Creating the app bundle as a part of the build also
    # debatably violates CMake principles, but hopefully not too much.

    # Copy resolved dependencies into app bundle
    get_target_property(IS_BUNDLE ${target} MACOSX_BUNDLE)
    if(IS_BUNDLE)
      add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ditto ${library_paths} "$<TARGET_BUNDLE_CONTENT_DIR:${target}>/Frameworks/"
        WORKING_DIRECTORY "$<TARGET_BUNDLE_CONTENT_DIR:${target}>"
        COMMENT "Copying dynamic libraries into app bundle"
      )
      # Add an rpath for the bundled dynamic libraries
      add_custom_command(
        TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path/../Frameworks/" $<TARGET_FILE:${target}>
        COMMENT "Adding rpath for dynamic libraries to binary"
      )
    endif()
  endif()
endfunction()
