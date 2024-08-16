#if defined(Hiro_ComboButton)

@implementation CocoaComboButton

-(id) initWith:(hiro::mComboButton&)comboButtonReference {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0) pullsDown:NO]) {
    comboButton = &comboButtonReference;

    [self setTarget:self];
    [self setAction:@selector(activate:)];
  }
  return self;
}

-(IBAction) activate:(id)sender {
  if(auto p = comboButton->self()) p->_updateSelected([self indexOfSelectedItem]);
  comboButton->doChange();
}

@end

namespace hiro {

auto pComboButton::construct() -> void {
  cocoaView = cocoaComboButton = [[CocoaComboButton alloc] initWith:self()];
  pWidget::construct();
}

auto pComboButton::destruct() -> void {
  [cocoaView removeFromSuperview];
}

auto pComboButton::append(sComboButtonItem item) -> void {
  [(CocoaComboButton*)cocoaView addItemWithTitle:[NSString stringWithUTF8String:item->text()]];
}

auto pComboButton::minimumSize() const -> Size {
  auto font = self().font(true);
  s32 maximumWidth = 0;
  for(auto& item : state().items) {
    maximumWidth = max(maximumWidth, pFont::size(font, item->state.text).width());
  }
  Size size = pFont::size(font, " ");
  return {maximumWidth + 36, size.height() + 6};
}

auto pComboButton::remove(sComboButtonItem item) -> void {
  [(CocoaComboButton*)cocoaView removeItemAtIndex:item->offset()];
}

auto pComboButton::reset() -> void {
  [(CocoaComboButton*)cocoaView removeAllItems];
}

auto pComboButton::setGeometry(Geometry geometry) -> void {
  pWidget::setGeometry({
    geometry.x() - 2, geometry.y(),
    geometry.width() + 4, geometry.height()
  });
}

auto pComboButton::_updateSelected(signed selected) -> void {
  for(auto& item : state().items) {
    item->state.selected = item->offset() == selected;
  }
}

}

#endif
