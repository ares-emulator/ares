target_sources(
  nall
  PRIVATE
    windows/detour.cpp
    windows/detour.hpp
    windows/registry.cpp
    windows/registry.hpp
    windows/service.hpp
    windows/utf8.cpp
    windows/utf8.hpp
    windows/windows.hpp
)

target_link_libraries(nall PUBLIC ws2_32 ole32 shell32 shlwapi)
if(MINGW)
  target_link_options(nall PUBLIC -mthreads -static)
endif()

set_source_files_properties(
  nall
  windows/detour.cpp
  windows/launcher.cpp
  windows/registry.cpp
  windows/utf8.cpp
  PROPERTIES HEADER_FILE_ONLY TRUE
)
