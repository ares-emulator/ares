target_enable_feature(hiro "Win32 UI")
target_compile_definitions(hiro PUBLIC HIRO_WINDOWS)
set(HIRO_BACKEND "Windows" PARENT_SCOPE)
target_link_libraries(
  hiro
  PRIVATE kernel32 user32 gdi32 advapi32 ole32 comctl32 comdlg32 uxtheme msimg32 dwmapi
)

set(hiro.resource windows/hiro.rc)

get_target_property(hiro_SOURCES hiro SOURCES)

target_sources(hiro PRIVATE cmake/os-windows.cmake)

set_source_files_properties(hiro ${hiro_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

set_source_files_properties(hiro hiro.cpp PROPERTIES HEADER_FILE_ONLY FALSE)
