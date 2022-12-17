#include <ruby/ruby.hpp>

#undef deprecated
#undef mkdir
#undef noinline
#undef usleep

#if defined(DISPLAY_XORG)
  #include <X11/Xlib.h>
  #include <X11/Xutil.h>
  #include <X11/Xatom.h>
  #include <X11/extensions/Xrandr.h>
#elif defined(DISPLAY_QUARTZ)
  #include <nall/macos/guard.hpp>
  #include <Cocoa/Cocoa.h>
  #include <Carbon/Carbon.h>
  #include <CoreFoundation/CoreFoundation.h>
  #include <IOKit/IOKitLib.h>
  #include <IOKit/graphics/IOGraphicsLib.h>
  #include <nall/macos/guard.hpp>
#elif defined(DISPLAY_WINDOWS)
  #include <nall/windows/windows.hpp>
  #include <initguid.h>
  #include <cguid.h>
  #include <mmsystem.h>
#endif

using namespace nall;
using namespace ruby;

#include <ruby/video/video.cpp>
#include <ruby/audio/audio.cpp>
#include <ruby/input/input.cpp>
