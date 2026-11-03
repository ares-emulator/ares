target_sources(
  ruby
  PRIVATE #
    video/direct3d9.cpp
    video/direct3d11.cpp
    video/direct3d11/d3d11device.cpp
    video/wgl.cpp
)

target_sources(
  ruby
  PRIVATE
    input/sdl.cpp
    input/shared/rawinput.cpp
    input/keyboard/rawinput.cpp
    input/mouse/rawinput.cpp
)

find_package(SDL)
find_package(librashader)

target_enable_feature(ruby "Direct3D 9 video driver" VIDEO_DIRECT3D9)
target_enable_feature(ruby "Direct3D 11 video driver" VIDEO_DIRECT3D11)
target_enable_feature(ruby "OpenGL video driver" VIDEO_WGL)
target_enable_feature(ruby "Windows input driver (XInput/DirectInput)" INPUT_WINDOWS)

if(SDL_FOUND)
  target_enable_feature(ruby "SDL input driver" INPUT_SDL)
  target_enable_feature(ruby "SDL audio driver" AUDIO_SDL)
endif()

if(librashader_FOUND AND ARES_ENABLE_LIBRASHADER)
  target_enable_feature(ruby "librashader OpenGL runtime" LIBRA_RUNTIME_OPENGL)
  target_enable_feature(ruby "librashader Direct3D11 runtime" LIBRA_RUNTIME_D3D11)
else()
  target_compile_definitions(ruby PRIVATE LIBRA_RUNTIME_OPENGL)
  target_compile_definitions(ruby PRIVATE LIBRA_RUNTIME_D3D11)
endif()

target_link_libraries(
  ruby
  PRIVATE
    $<$<BOOL:TRUE>:librashader::librashader>
    $<$<BOOL:${SDL_FOUND}>:SDL::SDL>
    d3d9
    d3d11
    d3dcompiler
    opengl32
    uuid
    avrt
    winmm
    ole32
    dinput8
    dxguid
	  ksuser
)
