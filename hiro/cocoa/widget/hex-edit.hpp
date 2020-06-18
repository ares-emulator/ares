#if defined(Hiro_HexEdit)

@interface CocoaHexEdit : NSScrollView {
@public
  hiro::mHexEdit* hexEdit;
}
-(id) initWith:(hiro::mHexEdit&)hexEdit;
@end

namespace hiro {

struct pHexEdit : public pWidget {
  Declare(HexEdit, Widget);

  auto setAddress(uint address) -> void;
  auto setBackgroundColor(Color color) -> void;
  auto setColumns(uint columns) -> void;
  auto setForegroundColor(Color color) -> void;
  auto setLength(uint length) -> void;
  auto setRows(uint rows) -> void;
  auto update() -> void;

  CocoaHexEdit* cocoaHexEdit = nullptr;
};

}

#endif
