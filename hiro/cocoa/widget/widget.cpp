#if defined(Hiro_Widget)

namespace hiro {

auto pWidget::construct() -> void {
  if(!cocoaView) {
    abstract = true;
    cocoaView = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 0, 0)];
    [cocoaView setHidden:true];
  }

  if(auto window = self().parentWindow(true)) {
    if(auto p = window->self()) p->_append(self());
    setDroppable(self().droppable());
    setEnabled(self().enabled(true));
    setFocusable(self().focusable());
    setFont(self().font(true));
    setMouseCursor(self().mouseCursor());
    setToolTip(self().toolTip());
    setVisible(self().visible(true));
  }
}

auto pWidget::destruct() -> void {
  [cocoaView removeFromSuperview];
}

auto pWidget::focused() const -> bool {
  return cocoaView == [[cocoaView window] firstResponder];
}

auto pWidget::setDroppable(bool droppable) -> void {
  //virtual
}

auto pWidget::setEnabled(bool enabled) -> void {
  if(abstract) enabled = false;

  if([cocoaView respondsToSelector:@selector(setEnabled:)]) {
    [(id)cocoaView setEnabled:enabled];
  }
}

auto pWidget::setFocusable(bool focusable) -> void {
  //virtual
}

auto pWidget::setFocused() -> void {
  [[cocoaView window] makeFirstResponder:cocoaView];
}

auto pWidget::setFont(const Font& font) -> void {
  if([cocoaView respondsToSelector:@selector(setFont:)]) {
    [(id)cocoaView setFont:pFont::create(font)];
  }
}

auto pWidget::setGeometry(Geometry geometry) -> void {
  CGFloat windowHeight = [[cocoaView superview] frame].size.height;
  //round coordinates
  u32 x = geometry.x();
  u32 y = windowHeight - geometry.y() - geometry.height();
  u32 width = geometry.width();
  u32 height = geometry.height();
  [cocoaView setFrame:NSMakeRect(x, y, width, height)];
  [[cocoaView superview] setNeedsDisplay:YES];
  pSizable::setGeometry(geometry);
}

auto pWidget::setMouseCursor(const MouseCursor& mouseCursor) -> void {
  //TODO
}

auto pWidget::setToolTip(const string& toolTip) -> void {
  //TODO
}

auto pWidget::setVisible(bool visible) -> void {
  if(abstract) visible = false;

  [cocoaView setHidden:!visible];
}

}

#endif
