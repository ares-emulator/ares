Home::Home(View* parent) : Panel(parent, Size{~0, ~0}) {
  setCollapsible().setVisible(false);
  image icon{Resource::Ares::Icon};
  icon.shrink();
  for(u32 y : range(icon.height())) {
    auto data = icon.data() + y * icon.pitch();
    for(u32 x : range(icon.width())) {
      u8 alpha = icon.read(data) >> 24;
      icon.write(data, u8(alpha * 0.15) << 24);
      data += icon.stride();
    }
  }
  icon.scale(sx(icon.width() * 0.75), sy(icon.height() * 0.75));
  iconCanvas.setIcon(icon);
}
