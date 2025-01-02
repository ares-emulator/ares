target_sources(
  ruby
  PRIVATE # cmake-format: sortable
    video/direct3d9.cpp
    video/wgl.cpp
)

target_sources(
  ruby
  PRIVATE # cmake-format: sortable
    audio/wasapi.cpp
    audio/xaudio2.cpp
    audio/xaudio2.hpp
    audio/directsound.cpp
    audio/waveout.cpp
    audio/sdl.cpp
)

target_sources(
  ruby
  PRIVATE # cmake-format: sortable
    input/sdl.cpp
    input/shared/rawinput.cpp
    input/windows.cpp
    input/keyboard/rawinput.cpp
    input/mouse/rawinput.cpp
    input/joypad/directinput.cpp
)

find_package(SDL)
find_package(librashader)

target_enable_feature(ruby "Direct3D 9 video driver" VIDEO_DIRECT3D9)
target_enable_feature(ruby "OpenGL video driver" VIDEO_WGL)
target_enable_feature(ruby "WASAPI audio driver" AUDIO_WASAPI)
target_enable_feature(ruby "XAudio2 audio driver" AUDIO_XAUDIO2)
target_enable_feature(ruby "DirectSound audio driver" AUDIO_DIRECTSOUND)
target_enable_feature(ruby "waveOut audio driver" AUDIO_WAVEOUT)
target_enable_feature(ruby "Windows input driver (XInput/DirectInput)" INPUT_WINDOWS)

if(SDL_FOUND)
  target_enable_feature(ruby "SDL input driver" INPUT_SDL)
  target_enable_feature(ruby "SDL audio driver" AUDIO_SDL)
endif()

if(librashader_FOUND AND ARES_ENABLE_LIBRASHADER)
  target_enable_feature(ruby "librashader OpenGL runtime" LIBRA_RUNTIME_OPENGL)
else()
  target_compile_definitions(ruby PRIVATE LIBRA_RUNTIME_OPENGL)
endif()

target_link_libraries(
  ruby
  PRIVATE
    $<$<BOOL:TRUE>:librashader::librashader>
    $<$<BOOL:${SDL_FOUND}>:SDL::SDL>
    d3d9
    opengl32
    dsound
    uuid
    avrt
    winmm
    ole32
    dinput8
    dxguid
)
