#if defined(Hiro_Monitor)

namespace hiro {

auto pMonitor::count() -> u32 {
  return [[NSScreen screens] count];
}

auto pMonitor::dpi(u32 monitor) -> Position {
  //macOS includes built-in HiDPI scaling support.
  //it may be better to rely on per-application scaling,
  //but for now we'll let macOS handle it so it works in all hiro applications.
  #if 0
  NSScreen* screen = [[NSScreen screens] objectAtIndex:monitor];
  NSDictionary* dictionary = [screen deviceDescription];
  NSSize dpi = [[dictionary objectForKey:NSDeviceSize] sizeValue];
  return {dpi.width, dpi.height};
  #endif
  return {96.0, 96.0};
}

auto pMonitor::geometry(u32 monitor) -> Geometry {
  NSRect rectangle = [[[NSScreen screens] objectAtIndex:monitor] frame];
  return {
    (s32)rectangle.origin.x,
    (s32)rectangle.origin.y,
    (s32)rectangle.size.width,
    (s32)rectangle.size.height
  };
}

auto pMonitor::primary() -> u32 {
  //on macOS, the primary monitor is always the first monitor.
  return 0;
}

auto pMonitor::workspace(u32 monitor) -> Geometry {
  NSRect size = [[[NSScreen screens] objectAtIndex:monitor] frame];
  NSRect area = [[[NSScreen screens] objectAtIndex:monitor] visibleFrame];
  return {
    (s32)area.origin.x,
    (s32)area.origin.y,
    (s32)area.size.width,
    (s32)area.size.height
  };
}

}

#endif
