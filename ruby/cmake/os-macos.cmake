find_package(SDL)
find_package(librashader)

if(XCODE AND WITH_SYSTEM_ZLIB)
  set_target_properties(ruby PROPERTIES OUTPUT_NAME "ruby-but-not-the-scripting-language")
endif()

target_sources(
  ruby
  PRIVATE video/cgl.cpp video/metal/metal.cpp video/metal/metal.hpp video/metal/Shaders.metal video/metal/ShaderTypes.h
)

# todo address
target_compile_options(ruby PRIVATE $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-unguarded-availability>)

target_sources(ruby PRIVATE audio/openal.cpp audio/sdl.cpp)

target_sources(
  ruby
  PRIVATE
    input/quartz.cpp
    input/keyboard/quartz.cpp
    input/sdl.cpp
    input/mouse/nsmouse.cpp
    input/joypad/iokit.cpp
    input/joypad/sdl.cpp
)

target_sources(ruby PRIVATE cmake/os-macos.cmake ruby.mm ruby.cpp)

target_link_libraries(
  ruby
  PRIVATE
    "$<LINK_LIBRARY:FRAMEWORK,CoreAudio.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,IOKit.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,QuartzCore.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,OpenAL.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,OpenGL.framework>"
    "$<LINK_LIBRARY:WEAK_FRAMEWORK,Metal.framework>"
    "$<LINK_LIBRARY:WEAK_FRAMEWORK,MetalKit.framework>"
)

if(SDL_FOUND)
  target_link_libraries(
    ruby
    PRIVATE "$<LINK_LIBRARY:WEAK_FRAMEWORK,SDL::SDL>"
  )
endif()

if(librashader_FOUND)
  target_link_libraries(
    ruby
    PRIVATE "$<LINK_LIBRARY:WEAK_LIBRARY,librashader::librashader>"
    # "$<$<BOOL:TRUE>:librashader::librashader>"
  )
endif()

target_enable_feature(ruby "OpenGL video driver" VIDEO_CGL)
target_enable_feature(ruby "Metal video driver" VIDEO_METAL)
target_enable_feature(ruby "OpenAL audio driver" AUDIO_OPENAL)
if(SDL_FOUND)
  target_enable_feature(ruby "SDL input driver" INPUT_SDL)
  target_enable_feature(ruby "SDL audio driver" AUDIO_SDL)
endif()
if(librashader_FOUND AND ARES_ENABLE_LIBRASHADER)
  target_enable_feature(ruby "librashader OpenGL runtime" LIBRA_RUNTIME_OPENGL)
  target_enable_feature(ruby "librashader Metal runtime" LIBRA_RUNTIME_METAL)
else()
  target_compile_definitions(ruby PRIVATE LIBRA_RUNTIME_OPENGL LIBRA_RUNTIME_METAL)
endif()
target_enable_feature(ruby "Quartz input driver" INPUT_QUARTZ)

add_custom_command(
  TARGET ruby
  POST_BUILD
  COMMAND $<$<CONFIG:Debug>:xcrun>
  ARGS -sdk macosx metal -o shaders.ir -c -gline-tables-only -frecord-sources Shaders.metal
  COMMAND $<$<CONFIG:Debug>:xcrun>
  ARGS -sdk macosx metallib -o shaders.metallib shaders.ir
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/video/metal
  COMMENT "Compiling debug .metallib for Metal debugging"
)
