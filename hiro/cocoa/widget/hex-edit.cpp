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
  cocoaView = cocoaHexEdit = [[CocoaHexEdit alloc] initWith:self()];
}

auto pHexEdit::destruct() -> void {
  [cocoaView removeFromSuperview];
}

auto pHexEdit::setAddress(u32 offset) -> void {
}

auto pHexEdit::setBackgroundColor(Color color) -> void {
}

auto pHexEdit::setColumns(u32 columns) -> void {
}

auto pHexEdit::setForegroundColor(Color color) -> void {
}

auto pHexEdit::setLength(u32 length) -> void {
}

auto pHexEdit::setRows(u32 rows) -> void {
}

auto pHexEdit::update() -> void {
}

}

#endif
