#if defined(Hiro_Desktop)

namespace hiro {

auto pDesktop::size() -> Size {
  NSRect primary = [[[NSScreen screens] objectAtIndex:0] frame];
  return {
    (s32)primary.size.width,
    (s32)primary.size.height
  };
}

auto pDesktop::workspace() -> Geometry {
  auto screen = Desktop::size();
  NSRect area = [[[NSScreen screens] objectAtIndex:0] visibleFrame];
  return {
    (s32)area.origin.x,
    (s32)area.origin.y,
    (s32)area.size.width,
    (s32)area.size.height
  };
}

}

#endif
