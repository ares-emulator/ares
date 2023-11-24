target_link_libraries(
  genius
  PRIVATE # cmake-format: sortable
    "$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>"
)

set_target_properties(
  genius
  PROPERTIES
    OUTPUT_NAME genius
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/data/genius.plist"
    XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY YES
    XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY YES
    XCODE_EMBED_PLUGINS_REMOVE_HEADERS_ON_COPY YES
    XCODE_EMBED_PLUGINS_CODE_SIGN_ON_COPY YES
)

# cmake-format: off
set_target_xcode_properties(
  genius
  PROPERTIES PRODUCT_BUNDLE_IDENTIFIER com.ares-emulator.genius
             PRODUCT_NAME genius
             ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
             CURRENT_PROJECT_VERSION ${ARES_BUILD_NUMBER}
             MARKETING_VERSION ${ARES_VERSION}
             GENERATE_INFOPLIST_FILE YES
             COPY_PHASE_STRIP NO
             CLANG_ENABLE_OBJC_ARC YES
             SKIP_INSTALL NO
             INSTALL_PATH "$(LOCAL_APPS_DIR)"
             INFOPLIST_KEY_CFBundleDisplayName "genius"
             INFOPLIST_KEY_NSHumanReadableCopyright "(c) 2004-${CURRENT_YEAR} ares team, Near et. al."
)
# cmake-format: on
