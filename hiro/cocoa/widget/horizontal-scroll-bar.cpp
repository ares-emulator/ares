#if defined(Hiro_HorizontalScrollBar)

@implementation CocoaHorizontalScrollBar : NSScroller

-(id) initWith:(hiro::mHorizontalScrollBar&)horizontalScrollBarReference {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 1, 0)]) {
    horizontalScrollBar = &horizontalScrollBarReference;

    [self setTarget:self];
    [self setAction:@selector(scroll:)];

    [self setControlSize:NSRegularControlSize];
    [self setScrollerStyle:NSScrollerStyleLegacy];
    [self setEnabled:YES];

    [self update];
  }
  return self;
}

-(void) update {
  f64 d = 1.0 / horizontalScrollBar->state.length;
  f64 f = d * horizontalScrollBar->state.position;

  [self setDoubleValue:f];
  [self setKnobProportion:d];
}

-(IBAction) scroll:(id)sender {
  auto& state = horizontalScrollBar->state;

  switch([self hitPart]) {
  case NSScrollerIncrementLine:
  case NSScrollerIncrementPage:
    if(state.position < state.length - 1) state.position++;
    [self update];
    break;

  case NSScrollerDecrementLine:
  case NSScrollerDecrementPage:
    if(state.position) state.position--;
    [self update];
    break;

  case NSScrollerKnob:
    state.position = [self doubleValue] * state.length;
    break;
  }

  horizontalScrollBar->doChange();
}

@end

namespace hiro {

auto pHorizontalScrollBar::construct() -> void {
  cocoaView = cocoaHorizontalScrollBar = [[CocoaHorizontalScrollBar alloc] initWith:self()];
  pWidget::construct();

  setLength(state().length);
  setPosition(state().position);
}

auto pHorizontalScrollBar::destruct() -> void {
  [cocoaView removeFromSuperview];
}

auto pHorizontalScrollBar::minimumSize() const -> Size {
  return {32, (s32)[NSScroller scrollerWidthForControlSize:NSRegularControlSize scrollerStyle:NSScrollerStyleLegacy]};
}

auto pHorizontalScrollBar::setLength(u32 length) -> void {
  [(CocoaHorizontalScrollBar*)cocoaView update];
}

auto pHorizontalScrollBar::setPosition(u32 position) -> void {
  [(CocoaHorizontalScrollBar*)cocoaView update];
}

}

#endif
