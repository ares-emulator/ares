#if defined(Hiro_HexEdit)

@interface CocoaHexEditTableView : NSTableView
@end

@interface CocoaHexEdit : NSScrollView <NSTableViewDataSource, NSTableViewDelegate> {
  // Not an NSTableViewDelegate because the table is cell-based, not view-based
@public
  hiro::mHexEdit* hexEdit; // this will be the data source for tableView.
  CocoaHexEditTableView* tableView;
}
-(id) initWith:(hiro::mHexEdit&)hexEdit;
-(NSTableView*) tableView; // helper function used in update()
-(hiro::mHexEdit*) hexEdit;
@end

namespace hiro {

struct pHexEdit : public pWidget {
  Declare(HexEdit, Widget);

  auto setAddress(u32 address) -> void;
  auto setBase(u8 base) -> void;
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
