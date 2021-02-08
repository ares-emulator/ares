#if defined(Hiro_Monitor)

namespace hiro {

auto pMonitor::count() -> u32 {
  return QApplication::desktop()->screenCount();
}

auto pMonitor::dpi(u32 monitor) -> Position {
  //Qt does not support per-monitor DPI retrieval
  return {
    QApplication::desktop()->logicalDpiX(),
    QApplication::desktop()->logicalDpiY()
  };
}

auto pMonitor::geometry(u32 monitor) -> Geometry {
  QRect rectangle = QApplication::desktop()->screenGeometry(monitor);
  return {rectangle.x(), rectangle.y(), rectangle.width(), rectangle.height()};
}

auto pMonitor::primary() -> u32 {
  return QApplication::desktop()->primaryScreen();
}

auto pMonitor::workspace(u32 monitor) -> Geometry {
  if(Monitor::count() == 1) {
    return Desktop::workspace();
  } else {
    return Monitor::geometry(monitor);
  }
}

}

#endif
