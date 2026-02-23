target_sources(
  ruby
  PRIVATE #
    video/glx.cpp
)

target_sources(
  ruby
  PRIVATE #
    audio/oss.cpp
    audio/alsa.cpp
    audio/openal.cpp
    audio/sdl.cpp
    audio/pulseaudio.cpp
    audio/pulseaudio-simple.cpp
    audio/ao.cpp
)

target_sources(
  ruby
  PRIVATE #
    input/xlib.cpp
    input/sdl.cpp
    input/mouse/xlib.cpp
    input/keyboard/xlib.cpp
    input/joypad/sdl.cpp
    input/joypad/udev.cpp
    input/joypad/uhid.cpp
    input/joypad/xinput.cpp
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

option(ARES_ENABLE_OPENAL "Enable the OpenAL audio driver" ON)
if(ARES_ENABLE_OPENAL)
  find_package(OpenAL)
endif()
if(OpenAL_FOUND)
  target_enable_feature(ruby "OpenAL audio driver" AUDIO_OPENAL)
else()
  target_disable_feature(ruby "OpenAL audio driver")
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

option(ARES_ENABLE_OSS "Enable the OSS audio driver" ON)
if(ARES_ENABLE_OSS)
  find_package(OSS)
endif()
if(OSS_FOUND)
  target_enable_feature(ruby "OSS audio driver" AUDIO_OSS)
else()
  target_disable_feature(ruby "OSS audio driver")
endif()

option(ARES_ENABLE_ALSA "Enable the ALSA audio driver" ON)
if(ARES_ENABLE_ALSA)
  find_package(ALSA)
endif()
if(ALSA_FOUND)
  target_enable_feature(ruby "ALSA audio driver" AUDIO_ALSA)
else()
  target_disable_feature(ruby "ALSA audio driver")
endif()

option(ARES_ENABLE_PULSEAUDIO "Enable the Pulse audio driver" ON)
if(ARES_ENABLE_PULSEAUDIO)
  find_package(PulseAudio)
endif()
if(PulseAudio_FOUND)
  target_enable_feature(ruby "PulseAudio audio driver" AUDIO_PULSEAUDIO AUDIO_PULSEAUDIOSIMPLE)
else()
  target_disable_feature(ruby "PulseAudio audio driver")
endif()

option(ARES_ENABLE_AO "Enable the AO audio driver" ON)
if(ARES_ENABLE_AO)
  find_package(AO)
endif()
if(AO_FOUND)
  target_enable_feature(ruby "AO audio driver" AUDIO_AO)
else()
  target_disable_feature(ruby "AO audio driver")
endif()

option(ARES_ENABLE_UDEV "Enable the udev input driver" ON)
if(ARES_ENABLE_UDEV)
  find_package(udev)
endif()
if(udev_FOUND)
  target_enable_feature(ruby "udev input driver" INPUT_UDEV)
else()
  target_disable_feature(ruby "udev input driver")
endif()

option(ARES_ENABLE_USBHID "Enable the usbhid input driver" ON)
if(ARES_ENABLE_USBHID)
  find_package(usbhid)
endif()
if(usbhid_FOUND)
  target_enable_feature(ruby "usbhid input driver" INPUT_UHID)
else()
  target_disable_feature(ruby "usbhid input driver")
endif()

target_link_libraries(
  ruby
  PRIVATE
    $<$<BOOL:${SDL_FOUND}>:SDL::SDL>
    $<$<BOOL:${OpenAL_FOUND}>:OpenAL::OpenAL>
    $<$<BOOL:TRUE>:librashader::librashader>
    $<$<BOOL:${OSS_FOUND}>:OSS::OSS>
    $<$<BOOL:${ALSA_FOUND}>:ALSA::ALSA>
    $<$<BOOL:${PulseAudio_FOUND}>:PulseAudio::PulseAudio>
    $<$<BOOL:${PulseAudio_FOUND}>:PulseAudio::PulseAudioSimple>
    $<$<BOOL:${AO_FOUND}>:AO::AO>
    $<$<BOOL:${udev_FOUND}>:udev::udev>
    $<$<BOOL:${usbhid_FOUND}>:usbhid::usbhid>
)
