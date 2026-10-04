if(armv6m IN_LIST ARES_COMPONENTS_LIST)
  add_executable(armv6m armv6m.cpp)

  target_include_directories(armv6m PRIVATE ${CMAKE_SOURCE_DIR})

  set(ARMV6M_SOURCE_DIR "${CMAKE_SOURCE_DIR}/ares/component/processor/armv6m")

  target_sources(
    armv6m
    PRIVATE
      ${ARMV6M_SOURCE_DIR}/armv6m.hpp
      ${ARMV6M_SOURCE_DIR}/decoder.cpp
      ${ARMV6M_SOURCE_DIR}/disassembler.cpp
      ${ARMV6M_SOURCE_DIR}/instruction.cpp
      ${ARMV6M_SOURCE_DIR}/serialization.cpp
  )

  set_target_properties(armv6m PROPERTIES FOLDER tests PREFIX "")
  get_target_property(armv6m_SOURCES armv6m SOURCES)
  list(REMOVE_ITEM armv6m_SOURCES armv6m.cpp)
  set_source_files_properties(${armv6m_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

  target_enable_subproject(armv6m "armv6m processor test harness")

  target_link_libraries(armv6m PRIVATE ares::ares ares::nall)
  set(CONSOLE TRUE)
  ares_configure_executable(armv6m)

  add_test(NAME armv6m-disassembler-contract COMMAND armv6m)
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES armv6m.cpp)
endif()
