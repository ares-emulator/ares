target_link_options(ares PRIVATE LINKER:-export-dynamic)

target_sources(ares PRIVATE cmake/os-freebsd.cmake)
