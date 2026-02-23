target_link_libraries(
  ares
  PRIVATE uuid kernel32 user32 gdi32 comctl32 comdlg32 shell32
)
target_sources(ares PRIVATE cmake/os-windows.cmake)
