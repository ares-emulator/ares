target_sources(nall PRIVATE macos/guard.hpp)

target_sources(nall PRIVATE cmake/os-macos.cmake)

target_link_libraries(nall PRIVATE "$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>")
