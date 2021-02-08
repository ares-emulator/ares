#if defined(Hiro_Monitor)

auto Monitor::count() -> u32 {
  return pMonitor::count();
}

auto Monitor::dpi(maybe<u32> monitor) -> Position {
  return pMonitor::dpi(monitor ? monitor() : primary());
}

auto Monitor::geometry(maybe<u32> monitor) -> Geometry {
  return pMonitor::geometry(monitor ? monitor() : primary());
}

auto Monitor::primary() -> u32 {
  return pMonitor::primary();
}

auto Monitor::workspace(maybe<u32> monitor) -> Geometry {
  return pMonitor::workspace(monitor ? monitor() : primary());
}

#endif
