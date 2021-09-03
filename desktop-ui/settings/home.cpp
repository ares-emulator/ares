auto HomePanel::construct() -> void {
  setCollapsible();
  setVisible(false);

  image icon{Resource::Ares::Icon1x};
  image icon2x{Resource::Ares::Icon2x};
  icon.shrink();
  icon2x.shrink();
  for(u32 y : range(icon.height())) {
    auto data = icon.data() + y * icon.pitch();
    for(u32 x : range(icon.width())) {
      u8 alpha = icon.read(data) >> 24;
      icon.write(data, u8(alpha * 0.15) << 24);
      data += icon.stride();
    }
  }
    
  for(u32 y : range(icon2x.height())) {
    auto data = icon2x.data() + y * icon2x.pitch();
    for(u32 x : range(icon2x.width())) {
      u8 alpha = icon2x.read(data) >> 24;
      icon2x.write(data, u8(alpha * 0.15) << 24);
      data += icon2x.stride();
    }
  }
    
  icon.scale(sx(icon.width() * 0.75), sy(icon.height() * 0.75));
  icon2x.scale(sx(icon2x.width() * 0.75), sy(icon2x.height() * 0.75));
  canvas.setIcon(multiFactorImage(icon, icon2x));
}
