target_sources(
  ruby
  PRIVATE #
    video/direct3d9.cpp
    video/direct3d12/d3d12device.cpp
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
target_enable_feature(ruby "Direct3D 12 video driver" VIDEO_DIRECT3D12)
target_enable_feature(ruby "OpenGL video driver" VIDEO_WGL)
target_enable_feature(ruby "Windows input driver (XInput/DirectInput)" INPUT_WINDOWS)

if(SDL_FOUND)
  target_enable_feature(ruby "SDL input driver" INPUT_SDL)
  target_enable_feature(ruby "SDL audio driver" AUDIO_SDL)
endif()

if(librashader_FOUND AND ARES_ENABLE_LIBRASHADER)
  target_enable_feature(ruby "librashader OpenGL runtime" LIBRA_RUNTIME_OPENGL)
  target_enable_feature(ruby "librashader Direct3D 12 runtime" LIBRA_RUNTIME_D3D12)
else()
  target_compile_definitions(ruby PRIVATE LIBRA_RUNTIME_OPENGL LIBRA_RUNTIME_D3D12)
endif()

target_link_libraries(
  ruby
  PRIVATE
    $<$<BOOL:TRUE>:librashader::librashader>
    $<$<BOOL:${SDL_FOUND}>:SDL::SDL>
    d3d9
    d3d12
    dxgi
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
