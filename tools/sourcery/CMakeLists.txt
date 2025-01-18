if(NOT (CMAKE_CROSSCOMPILING OR ARES_CROSSCOMPILING))
  add_executable(sourcery sourcery.cpp)
  export(TARGETS sourcery FILE "${CMAKE_BINARY_DIR}/sourceryConfig.cmake")

  target_include_directories(sourcery PRIVATE ${CMAKE_SOURCE_DIR})

  target_link_libraries(sourcery PRIVATE nall "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>>")

  set(CONSOLE TRUE)
  ares_configure_executable(sourcery)

  target_enable_subproject(sourcery "sourcery (resource compiler)")

  set_target_properties(sourcery PROPERTIES FOLDER tools PREFIX "")
else()
  set(sourcery_DIR ${CMAKE_SOURCE_DIR}/build_native)
  find_package(sourcery)
endif()
