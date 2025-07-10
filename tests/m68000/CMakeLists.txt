if(m68000 IN_LIST ARES_COMPONENTS_LIST)
  add_executable(m68000 m68000.cpp)

  target_include_directories(m68000 PRIVATE ${CMAKE_SOURCE_DIR})

  set(M68000_SOURCE_DIR "${CMAKE_SOURCE_DIR}/ares/component/processor/m68000")

  target_sources(
    m68000
    PRIVATE
      ${M68000_SOURCE_DIR}/algorithms.cpp
      ${M68000_SOURCE_DIR}/conditions.cpp
      ${M68000_SOURCE_DIR}/disassembler.cpp
      ${M68000_SOURCE_DIR}/effective-address.cpp
      ${M68000_SOURCE_DIR}/instruction.cpp
      ${M68000_SOURCE_DIR}/instructions.cpp
      ${M68000_SOURCE_DIR}/m68000.hpp
      ${M68000_SOURCE_DIR}/memory.cpp
      ${M68000_SOURCE_DIR}/registers.cpp
      ${M68000_SOURCE_DIR}/serialization.cpp
      ${M68000_SOURCE_DIR}/traits.cpp
      clone-tests.sh
      run-tests.sh
  )

  set_target_properties(m68000 PROPERTIES FOLDER tests PREFIX "")
  get_target_property(m68k_SOURCES m68000 SOURCES)
  list(REMOVE_ITEM m68k_SOURCES m68000.cpp)
  set_source_files_properties(${m68k_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

  target_enable_subproject(m68000 "m68000 processor test harness")

  target_link_libraries(m68000 PRIVATE ares::ares)
  set(CONSOLE TRUE)
  ares_configure_executable(m68000)

  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES m68000.cpp)
endif()
