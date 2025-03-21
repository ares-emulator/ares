include_guard(GLOBAL)

include(GNUInstallDirs)

include(dependencies)

set(ARES_BUILD_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/rundir")
set(ARES_BUILD_EXECUTABLE_DESTINATION ".")
set(ARES_INSTALL_EXECUTABLE_DESTINATION "${CMAKE_INSTALL_BINDIR}")
set(ARES_BUILD_DATA_DESTINATION ".")
set(ARES_INSTALL_DATA_DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/ares")
