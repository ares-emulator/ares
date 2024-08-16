#if defined(Hiro_Button)

@implementation CocoaButton

-(id) initWith:(hiro::mButton&)buttonReference {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
    button = &buttonReference;
    [self setTarget:self];
    [self setAction:@selector(activate:)];
    //NSRoundedBezelStyle has a fixed height; which breaks both icons and larger/smaller text
    [self setBezelStyle:NSRegularSquareBezelStyle];
  }
  return self;
}

-(IBAction) activate:(id)sender {
  button->doActivate();
}

@end

namespace hiro {

auto pButton::construct() -> void {
  cocoaView = cocoaButton = [[CocoaButton alloc] initWith:self()];
  pWidget::construct();

  setBordered(state().bordered);
  setIcon(state().icon);
  setOrientation(state().orientation);
  setText(state().text);
}

auto pButton::destruct() -> void {
  [cocoaView removeFromSuperview];
}

auto pButton::minimumSize() const -> Size {
  Size size = pFont::size(self().font(true), state().text);

  if(state().orientation == Orientation::Horizontal) {
    size.setWidth(size.width() + state().icon.width());
    size.setHeight(max(size.height(), state().icon.height()));
  }

  if(state().orientation == Orientation::Vertical) {
    size.setWidth(max(size.width(), state().icon.width()));
    size.setHeight(size.height() + state().icon.height());
  }

  return {size.width() + (state().text ? 20 : 8), size.height() + 8};
}

auto pButton::setBordered(bool bordered) -> void {
}

auto pButton::setGeometry(Geometry geometry) -> void {
  pWidget::setGeometry({
    geometry.x() - 2, geometry.y() - 2,
    geometry.width() + 4, geometry.height() + 4
  });
}

auto pButton::setIcon(const multiFactorImage& icon) -> void {
  [(CocoaButton*)cocoaView setImage:NSMakeImage(icon)];
}

auto pButton::setOrientation(Orientation orientation) -> void {
  if(orientation == Orientation::Horizontal) [(CocoaButton*)cocoaView setImagePosition:NSImageLeft];
  if(orientation == Orientation::Vertical  ) [(CocoaButton*)cocoaView setImagePosition:NSImageAbove];
}

auto pButton::setText(const string& text) -> void {
  [(CocoaButton*)cocoaView setTitle:[NSString stringWithUTF8String:text]];
}

}

#endif
