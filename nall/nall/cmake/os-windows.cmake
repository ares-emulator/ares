target_sources(
  nall
  PRIVATE
    windows/detour.cpp
    windows/detour.hpp
    windows/guid.cpp
    windows/guid.hpp
    windows/launcher.cpp
    windows/launcher.hpp
    windows/registry.cpp
    windows/registry.hpp
    windows/service.hpp
    windows/shared-memory.hpp
    windows/utf8.cpp
    windows/utf8.hpp
    windows/windows.hpp
)

if(MSVC)
  target_compile_options(nall PRIVATE /W2)
endif()

target_link_libraries(nall PUBLIC ws2_32 ole32 shell32 shlwapi)
if(MINGW)
  target_link_options(nall PUBLIC -mthreads -static)
endif()
target_link_libraries(nall PUBLIC ws2_32 ole32 shell32 shlwapi)

set_source_files_properties(
  nall
  windows/detour.cpp
  windows/launcher.cpp
  windows/registry.cpp
  windows/utf8.cpp
  PROPERTIES HEADER_FILE_ONLY TRUE
)
