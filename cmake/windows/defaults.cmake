set(ARES_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(ARES_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

set(ARES_EXECUTABLE_DESTINATION ${ARES_OUTPUT_DIR})
set(ARES_LIBRARY_DESTINATION lib)
set(ARES_INCLUDE_DESTINATION include)
# Set relative paths used by ares for self-discovery
set(ARES_DATA_PATH "../../${ARES_DATA_DESTINATION}")

set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL TRUE)

# todo, fix: under MSYS2, CMake looks for .dll.a files rather than .lib files; tell it to also look for .lib files
set(CMAKE_FIND_LIBRARY_SUFFIXES .lib ${CMAKE_FIND_LIBRARY_SUFFIXES})
# similarly; tell it to look for un-prefixed libraries on MSYS2
set(CMAKE_FIND_LIBRARY_PREFIXES ";lib" ${CMAKE_FIND_LIBRARY_SUFFIXES})

include(dependencies)
