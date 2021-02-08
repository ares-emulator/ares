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

  auto setAddress(u32 address) -> void;
  auto setBackgroundColor(Color color) -> void;
  auto setColumns(u32 columns) -> void;
  auto setForegroundColor(Color color) -> void;
  auto setLength(u32 length) -> void;
  auto setRows(u32 rows) -> void;
  auto update() -> void;

  CocoaHexEdit* cocoaHexEdit = nullptr;
};

}

#endif
