#if defined(Hiro_Monitor)

//per-monitor API is only on Windows 10+

namespace hiro {

struct MonitorInfo {
  u32 monitor = 0;
  u32 primary = 0;
  Geometry geometry;
  u32 index = 0;
};

static auto CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
  MonitorInfo& info = *(MonitorInfo*)dwData;
  MONITORINFOEX mi{};
  mi.cbSize = sizeof(MONITORINFOEX);
  GetMonitorInfo(hMonitor, &mi);
  string displayName = (const char*)utf8_t(mi.szDevice);
  if(displayName.beginsWith(R"(\\.\DISPLAYV)")) return true;  //ignore pseudo-monitors
  if(mi.dwFlags & MONITORINFOF_PRIMARY) info.primary = info.index;
  if(info.monitor == info.index) {
    info.geometry = {lprcMonitor->left, lprcMonitor->top, lprcMonitor->right - lprcMonitor->left, lprcMonitor->bottom - lprcMonitor->top};
  }
  info.index++;
  return true;
}

auto pMonitor::count() -> u32 {
  return GetSystemMetrics(SM_CMONITORS);
}

auto pMonitor::dpi(uint monitor) -> Position {
  HDC hdc = GetDC(0);
  auto dpiX = (f32)GetDeviceCaps(hdc, LOGPIXELSX);
  auto dpiY = (f32)GetDeviceCaps(hdc, LOGPIXELSY);
  ReleaseDC(0, hdc);
  return {dpiX, dpiY};
}

auto pMonitor::geometry(u32 monitor) -> Geometry {
  MonitorInfo info;
  info.monitor = monitor;
  EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, (LPARAM)&info);
  return info.geometry;
}

auto pMonitor::primary() -> u32 {
  MonitorInfo info;
  EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, (LPARAM)&info);
  return info.primary;
}

auto pMonitor::workspace(u32 monitor) -> Geometry {
  return pDesktop::workspace();
}

}

#endif
