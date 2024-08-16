#if defined(Hiro_TableView)

@class CocoaTableViewContent;

@interface CocoaTableView : NSScrollView <NSTableViewDelegate, NSTableViewDataSource> {
@public
  hiro::mTableView* tableView;
  CocoaTableViewContent* content;
  NSFont* font;
}
-(id) initWith:(hiro::mTableView&)tableViewReference;
-(CocoaTableViewContent*) content;
-(NSFont*) font;
-(void) setFont:(NSFont*)font;
-(void) reloadColumns;
-(NSInteger) numberOfRowsInTableView:(NSTableView*)table;
-(id) tableView:(NSTableView*)table objectValueForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row;
-(BOOL) tableView:(NSTableView*)table shouldShowCellExpansionForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row;
-(NSString*) tableView:(NSTableView*)table toolTipForCell:(NSCell*)cell rect:(NSRectPointer)rect tableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row mouseLocation:(NSPoint)mouseLocation;
-(void) tableView:(NSTableView*)tableView willDisplayCell:(id)cell forTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row;
-(void) tableViewSelectionDidChange:(NSNotification*)notification;
-(IBAction) doubleAction:(id)sender;
@end

@interface CocoaTableViewContent : NSTableView {
  hiro::mTableView* tableView;
}
-(id) initWith:(hiro::mTableView&)tableViewReference;
-(void) reloadData;
-(void) keyDown:(NSEvent*)event;
-(NSMenu*) menuForEvent:(NSEvent*)event;
@end

@interface CocoaTableViewCell : NSCell {
  hiro::mTableView* tableView;
  NSButtonCell* buttonCell;
  NSTextFieldCell* textCell;
}
-(id) initWith:(hiro::mTableView&)tableViewReference;
-(NSString*) stringValue;
-(void) drawWithFrame:(NSRect)frame inView:(NSView*)view;
-(NSUInteger) hitTestForEvent:(NSEvent*)event inRect:(NSRect)frame ofView:(NSView*)view;
-(BOOL) trackMouse:(NSEvent*)event inRect:(NSRect)frame ofView:(NSView*)view untilMouseUp:(BOOL)flag;
+(BOOL) prefersTrackingUntilMouseUp;
@end

namespace hiro {

struct pTableView : pWidget {
  Declare(TableView, Widget)

  auto append(sTableViewColumn column) -> void;
  auto append(sTableViewItem item) -> void;
  auto remove(sTableViewColumn column) -> void;
  auto remove(sTableViewItem item) -> void;
  auto resizeColumns() -> void;
  auto setAlignment(Alignment alignment) -> void;
  auto setBackgroundColor(Color color) -> void;
  auto setBatchable(bool batchable) -> void;
  auto setBordered(bool bordered) -> void;
  auto setEnabled(bool enabled) -> void override;
  auto setFont(const Font& font) -> void override;
  auto setForegroundColor(Color color) -> void;
  auto setHeadered(bool headered) -> void;
  auto setSortable(bool sortable) -> void;
  auto setUsesSidebarStyle(bool usesSidebarStyle) -> void;

  auto _cellWidth(u32 row, u32 column) -> u32;
  auto _columnWidth(u32 column) -> u32;
  auto _width(u32 column) -> u32;

  CocoaTableView* cocoaTableView = nullptr;
};

}

#endif
