#if defined(Hiro_HexEdit)

@implementation CocoaHexEdit : NSScrollView

-(id) initWith:(hiro::mHexEdit&)hexEditReference {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
    hexEdit = &hexEditReference;
  }
  return self;
}

@end

namespace hiro {

auto pHexEdit::construct() -> void {
  @autoreleasepool {
    cocoaView = cocoaHexEdit = [[CocoaHexEdit alloc] initWith:self()];
  }
}

auto pHexEdit::destruct() -> void {
  @autoreleasepool {
    [cocoaView removeFromSuperview];
    [cocoaView release];
  }
}

auto pHexEdit::setAddress(uint offset) -> void {
}

auto pHexEdit::setBackgroundColor(Color color) -> void {
}

auto pHexEdit::setColumns(uint columns) -> void {
}

auto pHexEdit::setForegroundColor(Color color) -> void {
}

auto pHexEdit::setLength(uint length) -> void {
}

auto pHexEdit::setRows(uint rows) -> void {
}

auto pHexEdit::update() -> void {
}

}

#endif
