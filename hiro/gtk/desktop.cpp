#if defined(Hiro_Desktop)

namespace hiro {

auto pDesktop::size() -> Size {
  return {
    gdk_screen_get_width(gdk_screen_get_default()),
    gdk_screen_get_height(gdk_screen_get_default())
  };
}

auto pDesktop::workspace() -> Geometry {
  #if defined(DISPLAY_WINDOWS)
  RECT rc;
  SystemParametersInfo(SPI_GETWORKAREA, 0, &rc, 0);
  return {rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top};
  #endif

  #if defined(DISPLAY_XORG)
  auto display = XOpenDisplay(nullptr);
  s32 screen = DefaultScreen(display);

  s32 format;
  unsigned char* data = nullptr;
  unsigned long items, after;
  XlibAtom returnAtom;

  XlibAtom netWorkarea = XInternAtom(display, "_NET_WORKAREA", XlibTrue);
  s32 result = XGetWindowProperty(
    display, RootWindow(display, screen), netWorkarea, 0, 4, XlibFalse,
    XInternAtom(display, "CARDINAL", XlibTrue), &returnAtom, &format, &items, &after, &data
  );

  XlibAtom cardinal = XInternAtom(display, "CARDINAL", XlibTrue);
  if(result == Success && returnAtom == cardinal && format == 32 && items == 4) {
    unsigned long* workarea = (unsigned long*)data;
    XCloseDisplay(display);
    return {(s32)workarea[0], (s32)workarea[1], (s32)workarea[2], (s32)workarea[3]};
  }

  XCloseDisplay(display);
  return {
    0, 0,
    gdk_screen_get_width(gdk_screen_get_default()),
    gdk_screen_get_height(gdk_screen_get_default())
  };
  #endif

  return {};
}

}

#endif
