target_compile_definitions(nall PUBLIC PLATFORM_LINUX)

target_sources(
  nall
  PRIVATE # cmake-format: sortable
    xorg/clipboard.hpp
    xorg/guard.hpp
    xorg/xorg.hpp
)
