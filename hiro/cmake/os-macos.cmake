target_sources(hiro PRIVATE cmake/os-macos.cmake hiro.mm hiro.cpp)

target_enable_feature(hiro "Cocoa UI backend" HIRO_COCOA)

target_link_libraries(
  hiro
  PRIVATE
    "$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,Carbon.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,IOKit.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,Security.framework>"
)

get_target_property(hiro_SOURCES hiro SOURCES)

set_source_files_properties(hiro ${hiro_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

if(${CMAKE_SYSTEM_NAME} STREQUAL Darwin)
  set_source_files_properties(hiro hiro.mm PROPERTIES HEADER_FILE_ONLY FALSE)
else()
  set_source_files_properties(hiro hiro.cpp PROPERTIES HEADER_FILE_ONLY FALSE)
endif()
