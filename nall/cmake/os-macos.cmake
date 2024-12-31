target_sources(nall PRIVATE macos/guard.hpp)

target_sources(nall PRIVATE cmake/os-macos.cmake)

target_compile_definitions(nall PUBLIC PLATFORM_MACOS)

target_link_libraries(nall PRIVATE "$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>")
target_compile_options(nall PRIVATE -Wno-error=switch)
