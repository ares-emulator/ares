#if defined(Hiro_VerticalSlider)

@implementation CocoaVerticalSlider 

-(id) initWith:(hiro::mVerticalSlider&)verticalSliderReference {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 1)]) {
    verticalSlider = &verticalSliderReference;

    [self setTarget:self];
    [self setAction:@selector(activate:)];
    [self setMinValue:0];
  }
  return self;
}

-(IBAction) activate:(id)sender {
  verticalSlider->state.position = [self doubleValue];
  verticalSlider->doChange();
}

@end

namespace hiro {

auto pVerticalSlider::construct() -> void {
  cocoaView = cocoaVerticalSlider = [[CocoaVerticalSlider alloc] initWith:self()];
  pWidget::construct();

  setLength(state().length);
  setPosition(state().position);
}

auto pVerticalSlider::destruct() -> void {
  [cocoaView removeFromSuperview];
}

auto pVerticalSlider::minimumSize() const -> Size {
  return {20, 48};
}

auto pVerticalSlider::setGeometry(Geometry geometry) -> void {
  pWidget::setGeometry({
    geometry.x(), geometry.y() - 2,
    geometry.width(), geometry.height() + 4
  });
}

auto pVerticalSlider::setLength(u32 length) -> void {
  [(CocoaVerticalSlider*)cocoaView setMaxValue:length - 1];
}

auto pVerticalSlider::setPosition(u32 position) -> void {
  [(CocoaVerticalSlider*)cocoaView setDoubleValue:position];
}

}

#endif
