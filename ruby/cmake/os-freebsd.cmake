target_sources(
  ruby
  PRIVATE #
    video/glx.cpp
)

target_sources(
  ruby
  PRIVATE #
    input/sdl.cpp
    input/mouse/xlib.cpp
    input/keyboard/xlib.cpp
    input/joypad/sdl.cpp
)

find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)

target_link_libraries(ruby PRIVATE X11::Xrandr OpenGL::GLX)

target_enable_feature(ruby "GLX OpenGL video driver" VIDEO_GLX)
target_enable_feature(ruby "Xlib input driver" INPUT_XLIB)

find_package(librashader)
if(librashader_FOUND AND ARES_ENABLE_LIBRASHADER)
  target_enable_feature(ruby "librashader OpenGL runtime" LIBRA_RUNTIME_OPENGL)
else()
  # continue to define the runtime so openGL compiles
  target_compile_definitions(ruby PRIVATE LIBRA_RUNTIME_OPENGL)
endif()

option(ARES_ENABLE_SDL "Enable SDL audio and input drivers" ON)
if(ARES_ENABLE_SDL)
  find_package(SDL)
endif()
if(SDL_FOUND)
  target_enable_feature(ruby "SDL input driver" INPUT_SDL)
  target_enable_feature(ruby "SDL audio driver" AUDIO_SDL)
else()
  target_disable_feature(ruby "SDL audio driver")
  target_disable_feature(ruby "SDL input driver")
endif()

target_link_libraries(
  ruby
  PRIVATE
    $<$<BOOL:${SDL_FOUND}>:SDL::SDL>
    $<$<BOOL:TRUE>:librashader::librashader>
)
