#if defined(Hiro_TableView)

@implementation CocoaTableView : NSScrollView

-(id) initWith:(hiro::mTableView&)tableViewReference {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
    tableView = &tableViewReference;
    content = [[CocoaTableViewContent alloc] initWith:tableViewReference];

    [self setDocumentView:content];
    [self setBorderType:NSBezelBorder];
    [self setHasVerticalScroller:YES];

    [content setDataSource:self];
    [content setDelegate:self];
    [content setTarget:self];
    [content setDoubleAction:@selector(doubleAction:)];

    [content setAllowsColumnReordering:NO];
    [content setAllowsColumnResizing:YES];
    [content setAllowsColumnSelection:NO];
    [content setAllowsEmptySelection:YES];
    [content setColumnAutoresizingStyle:NSTableViewLastColumnOnlyAutoresizingStyle];

    font = nil;
    [self setFont:nil];
  }
  return self;
}

-(void) setUsesSidebarStyle:(bool)usesSidebarStyle {
  content.selectionHighlightStyle = usesSidebarStyle? NSTableViewSelectionHighlightStyleSourceList : NSTableViewSelectionHighlightStyleRegular;
  self.borderType = usesSidebarStyle? NSNoBorder : NSBezelBorder;
}

-(bool) usesSidebarStyle {
  return content.selectionHighlightStyle == NSTableViewSelectionHighlightStyleSourceList;
}

-(CocoaTableViewContent*) content {
  return content;
}

-(NSFont*) font {
  return font;
}

-(void) setFont:(NSFont*)fontPointer {
  if(!fontPointer) fontPointer = [NSFont systemFontOfSize:12];
  font = fontPointer;

  u32 fontHeight = hiro::pFont::size(font, " ").height();
  [content setFont:font];
  [content setRowHeight:fontHeight + 4];
  [self reloadColumns];
  tableView->resizeColumns();
}

-(void) reloadColumns {
  while([[content tableColumns] count]) {
    [content removeTableColumn:[[content tableColumns] lastObject]];
  }

  for(auto& tableViewColumn : tableView->state.columns) {
    auto column = tableViewColumn->offset();

    NSTableColumn* tableColumn = [[NSTableColumn alloc] initWithIdentifier:[[NSNumber numberWithInteger:column] stringValue]];
    NSTableHeaderCell* headerCell = [[NSTableHeaderCell alloc] initTextCell:[NSString stringWithUTF8String:tableViewColumn->state.text]];
    CocoaTableViewCell* dataCell = [[CocoaTableViewCell alloc] initWith:*tableView];

    [dataCell setEditable:NO];

    [tableColumn setResizingMask:NSTableColumnAutoresizingMask | NSTableColumnUserResizingMask];
    [tableColumn setHeaderCell:headerCell];
    [tableColumn setDataCell:dataCell];

    [content addTableColumn:tableColumn];
  }
}

-(NSInteger) numberOfRowsInTableView:(NSTableView*)table {
  return tableView->state.items.size();
}

-(id) tableView:(NSTableView*)table objectValueForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
  if(auto tableViewItem = tableView->item(row)) {
    if(auto tableViewCell = tableViewItem->cell([[tableColumn identifier] integerValue])) {
      NSString* text = [NSString stringWithUTF8String:tableViewCell->state.text];
      return @{ @"text":text };  //used by type-ahead
    }
  }
  return @{};
}

-(BOOL) tableView:(NSTableView*)table shouldShowCellExpansionForTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
  return NO;
}

-(NSString*) tableView:(NSTableView*)table toolTipForCell:(NSCell*)cell rect:(NSRectPointer)rect tableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row mouseLocation:(NSPoint)mouseLocation {
  return nil;
}

-(void) tableView:(NSTableView*)tableView willDisplayCell:(id)cell forTableColumn:(NSTableColumn*)tableColumn row:(NSInteger)row {
  [cell setFont:[self font]];
}

-(void) tableViewSelectionDidChange:(NSNotification*)notification {
  if(tableView->self()->locked()) return;
  for(auto& tableViewItem : tableView->state.items) {
    tableViewItem->state.selected = tableViewItem->offset() == [content selectedRow];
  }
  tableView->doChange();
}

-(IBAction) doubleAction:(id)sender {
  s32 row = [content clickedRow];
  if(row >= 0 && row < tableView->state.items.size()) {
    s32 column = [content clickedColumn];
    if(column >= 0 && column < tableView->state.columns.size()) {
      auto item = tableView->state.items[row];
      auto cell = item->cell(column);
      tableView->doActivate(cell);
    }
  }
}

@end

@implementation CocoaTableViewContent : NSTableView

-(id) initWith:(hiro::mTableView&)tableViewReference {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
    tableView = &tableViewReference;
  }
  return self;
}

-(void) reloadData {
  //acquire lock to prevent tableViewSelectionDidChange from invoking the onChange callback
  auto lock = tableView->self()->acquire();
  [super reloadData];
}

-(void) keyDown:(NSEvent*)event {
  auto character = [[event characters] characterAtIndex:0];
  if(character == NSEnterCharacter || character == NSCarriageReturnCharacter) {
    s32 row = [self selectedRow];
    if(row >= 0 && row < tableView->state.items.size()) {
      s32 column = max(0, [self selectedColumn]);  //can be -1?
      if(column >= 0 && column < tableView->state.columns.size()) {
        auto item = tableView->state.items[row];
        auto cell = item->cell(column);
        tableView->doActivate(cell);
      }
    }
  }

  [super keyDown:event];
}

-(NSMenu*) menuForEvent:(NSEvent*)event {
  //macOS doesn't set focus to right-clicked items, but this is neccesary for context menus:
  //todo: select the current column as well so that doContext(cell) works correctly
  NSInteger row = [self rowAtPoint:[self convertPoint:event.locationInWindow fromView:nil]];
  if(row >= 0 && ![self isRowSelected:row]) {
    [self selectRowIndexes:[NSIndexSet indexSetWithIndex:row] byExtendingSelection:NO];
  }

#if 0 // breaks the build
  s32 row = [content clickedRow];
  if(row >= 0 && _row < tableView->state.items.size()) {
    s32 column = [content clickedColumn];
    if(column >= 0 && column < tableView->state.columns.size()) {
      auto item = tableView->state.items[row];
      auto cell = item->cell(column);
      tableView->doContext(cell);
    }
  }
#endif
  return nil;
}

- (void)drawRect:(NSRect)dirtyRect
{
  [super drawRect:dirtyRect];
  if(((CocoaTableView*)self.enclosingScrollView).usesSidebarStyle) {
    NSRect frame = self.bounds;
    frame.origin.x += frame.size.width - 1;
    frame.size.width = 1;
    if(@available(macOS 10.14, *)) [[NSColor separatorColor] set];
    else if (@available(macOS 10.10, *)) [[NSColor secondaryLabelColor] set];
    else [[NSColor shadowColor] set];
    [NSBezierPath fillRect:frame];
  }
}

@end

@implementation CocoaTableViewCell : NSCell

-(id) initWith:(hiro::mTableView&)tableViewReference {
  if(self = [super initTextCell:@""]) {
    tableView = &tableViewReference;
    buttonCell = [[NSButtonCell alloc] initTextCell:@""];
    [buttonCell setButtonType:NSSwitchButton];
    [buttonCell setControlSize:NSSmallControlSize];
    [buttonCell setRefusesFirstResponder:YES];
    [buttonCell setTarget:self];
    textCell = [[NSTextFieldCell alloc] init];
  }
  return self;
}

//used by type-ahead
-(NSString*) stringValue {
  return [[self objectValue] objectForKey:@"text"];
}

-(void) drawWithFrame:(NSRect)frame inView:(NSView*)_view {
  NSTableView* view = (NSTableView*)_view;
  if(auto tableViewItem = tableView->item([view rowAtPoint:frame.origin])) {
    if(auto tableViewCell = tableViewItem->cell([view columnAtPoint:frame.origin])) {
      if(tableViewCell->state.checkable) {
        [buttonCell setHighlighted:YES];
        [buttonCell setState:(tableViewCell->state.checked ? NSOnState : NSOffState)];
        [buttonCell drawWithFrame:frame inView:view];
        frame.origin.x += frame.size.height + 2;
        frame.size.width -= frame.size.height + 2;
      }

      if(tableViewCell->state.icon) {
        NSImage* image = NSMakeImage(tableViewCell->state.icon,
                                     tableViewCell->state.icon.width(),
                                     tableViewCell->state.icon.height());
        [[NSGraphicsContext currentContext] saveGraphicsState];
        NSRect targetRect = NSMakeRect(frame.origin.x + 2, frame.origin.y + (frame.size.height - image.size.height) / 2,
                                       image.size.width, image.size.height);
        NSRect sourceRect = NSMakeRect(0, 0, image.size.width, image.size.height);
        [image drawInRect:targetRect fromRect:sourceRect operation:NSCompositeSourceOver fraction:1.0 respectFlipped:YES hints:nil];
        [[NSGraphicsContext currentContext] restoreGraphicsState];
        frame.origin.x += image.size.width + 4;
        frame.size.width -= image.size.width + 4;
      }

      if(tableViewCell->state.text) {
        textCell.stringValue = @((const char*)tableViewCell->state.text);
        textCell.alignment = NSTextAlignmentCenter;
        textCell.backgroundStyle = self.backgroundStyle;
        textCell.font = hiro::pFont::create(tableViewCell->font(true));
        if(tableViewCell->state.alignment.horizontal() < 0.333) textCell.alignment = NSTextAlignmentLeft;
        if(tableViewCell->state.alignment.horizontal() > 0.666) textCell.alignment = NSTextAlignmentRight;
        textCell.textColor = nil;
        if(![self isHighlighted] && tableView->enabled(true)) {
          auto systemColor = tableViewCell->state.foregroundSystemColor;
          if(systemColor != hiro::SystemColor::None) textCell.textColor = NSMakeColor(systemColor);
          else if(auto color = tableViewCell->state.foregroundColor) textCell.textColor = NSMakeColor(color);
        }
        [textCell drawWithFrame:frame inView:view];
      }
    }
  }
}

//needed to trigger trackMouse events
-(NSUInteger) hitTestForEvent:(NSEvent*)event inRect:(NSRect)frame ofView:(NSView*)view {
  NSUInteger hitTest = [super hitTestForEvent:event inRect:frame ofView:view];
  NSPoint point = [view convertPoint:[event locationInWindow] fromView:nil];
  NSRect rect = NSMakeRect(frame.origin.x, frame.origin.y, frame.size.height, frame.size.height);
  if(NSMouseInRect(point, rect, [view isFlipped])) {
    hitTest |= NSCellHitTrackableArea;
  }
  return hitTest;
}

//I am unable to get startTrackingAt:, continueTracking:, stopTracking: to work
//so instead, I have to run a modal loop on events until the mouse button is released
-(BOOL) trackMouse:(NSEvent*)event inRect:(NSRect)frame ofView:(NSView*)_view untilMouseUp:(BOOL)flag {
  NSTableView* view = (NSTableView*)_view;
  if([event type] == NSLeftMouseDown) {
    NSWindow* window = [view window];
    NSEvent* nextEvent;
    while((nextEvent = [window nextEventMatchingMask:(NSLeftMouseDragged | NSLeftMouseUp)])) {
      if([nextEvent type] == NSLeftMouseUp) {
        NSPoint point = [view convertPoint:[nextEvent locationInWindow] fromView:nil];
        NSRect rect = NSMakeRect(frame.origin.x, frame.origin.y, frame.size.height, frame.size.height);
        if(NSMouseInRect(point, rect, [view isFlipped])) {
          if(auto tableViewItem = tableView->item([view rowAtPoint:point])) {
            if(auto tableViewCell = tableViewItem->cell([view columnAtPoint:point])) {
              tableViewCell->state.checked = !tableViewCell->state.checked;
              tableView->doToggle(tableViewCell->instance);
            }
          }
        }
        break;
      }
    }
  }
  return YES;
}

+(BOOL) prefersTrackingUntilMouseUp {
  return YES;
}

@end

namespace hiro {

auto pTableView::construct() -> void {
  cocoaView = cocoaTableView = [[CocoaTableView alloc] initWith:self()];
  pWidget::construct();

  setAlignment(state().alignment);
  setBackgroundColor(state().backgroundColor);
  setBatchable(state().batchable);
  setBordered(state().bordered);
  setFont(self().font(true));
  setForegroundColor(state().foregroundColor);
  setHeadered(state().headered);
  setSortable(state().sortable);
}

auto pTableView::destruct() -> void {
  [cocoaView removeFromSuperview];
}

auto pTableView::append(sTableViewColumn column) -> void {
  [(CocoaTableView*)cocoaView reloadColumns];
  resizeColumns();
}

auto pTableView::append(sTableViewItem item) -> void {
  [[(CocoaTableView*)cocoaView content] reloadData];
}

auto pTableView::remove(sTableViewColumn column) -> void {
  [(CocoaTableView*)cocoaView reloadColumns];
  resizeColumns();
}

auto pTableView::remove(sTableViewItem item) -> void {
  [[(CocoaTableView*)cocoaView content] reloadData];
}

auto pTableView::resizeColumns() -> void {
  vector<s32> widths;
  s32 minimumWidth = 0;
  s32 expandable = 0;
  for(u32 column : range(self().columnCount())) {
    s32 width = _width(column);
    widths.append(width);
    minimumWidth += width;
    if(state().columns[column]->expandable()) expandable++;
  }

  s32 maximumWidth = self().geometry().width() - 18;  //include margin for vertical scroll bar
  s32 expandWidth = 0;
  if(expandable && maximumWidth > minimumWidth) {
    expandWidth = (maximumWidth - minimumWidth) / expandable;
  }

  for(u32 column : range(self().columnCount())) {
    if(auto self = state().columns[column]->self()) {
      s32 width = widths[column];
      if(self->state().expandable) width += expandWidth;
      NSTableColumn* tableColumn = [[(CocoaTableView*)cocoaView content] tableColumnWithIdentifier:[[NSNumber numberWithInteger:column] stringValue]];
      [tableColumn setWidth:width];
    }
  }
}

auto pTableView::setAlignment(Alignment alignment) -> void {
}

auto pTableView::setBackgroundColor(Color color) -> void {
}

auto pTableView::setBatchable(bool batchable) -> void {
  [[(CocoaTableView*)cocoaView content] setAllowsMultipleSelection:(batchable ? YES : NO)];
}

auto pTableView::setBordered(bool bordered) -> void {
}

auto pTableView::setEnabled(bool enabled) -> void {
  pWidget::setEnabled(enabled);
  [[(CocoaTableView*)cocoaView content] setEnabled:enabled];
}

auto pTableView::setFont(const Font& font) -> void {
  [(CocoaTableView*)cocoaView setFont:pFont::create(font)];
}

auto pTableView::setForegroundColor(Color color) -> void {
}

auto pTableView::setHeadered(bool headered) -> void {
  if(headered) {
    [[(CocoaTableView*)cocoaView content] setHeaderView:[[NSTableHeaderView alloc] init]];
  } else {
    [[(CocoaTableView*)cocoaView content] setHeaderView:nil];
  }
}

auto pTableView::setSortable(bool sortable) -> void {
  //TODO
}

auto pTableView::setUsesSidebarStyle(bool usesSidebarStyle) -> void {
  ((CocoaTableView*)cocoaView).usesSidebarStyle = usesSidebarStyle;
}

auto pTableView::_cellWidth(u32 row, u32 column) -> u32 {
  u32 width = 8;
  if(auto pTableViewItem = self().item(row)) {
    if(auto pTableViewCell = pTableViewItem->cell(column)) {
      if(pTableViewCell->state.checkable) {
        width += 24;
      }
      if(auto& icon = pTableViewCell->state.icon) {
        width += icon.width() + 2;
      }
      if(auto& text = pTableViewCell->state.text) {
        width += pFont::size(pTableViewCell->font(true), text).width();
      }
    }
  }
  return width;
}

auto pTableView::_columnWidth(u32 column_) -> u32 {
  u32 width = 8;
  if(auto column = self().column(column_)) {
    if(auto& icon = column->state.icon) {
      width += icon.width() + 2;
    }
    if(auto& text = column->state.text) {
      width += pFont::size(column->font(true), text).width();
    }
    if(column->state.sorting != Sort::None) {
      width += 16;
    }
  }
  return width;
}

auto pTableView::_width(u32 column) -> u32 {
  if(auto width = self().column(column).width()) return width;
  u32 width = 1;
  if(!self().column(column).visible()) return width;
  if(state().headered) width = max(width, _columnWidth(column));
  for(auto row : range(state().items.size())) {
    width = max(width, _cellWidth(row, column));
  }
  return width;
}

/*
auto pTableView::setSelected(bool selected) -> void {
  if(selected == false) {
    [[cocoaView content] deselectAll:nil];
  }
}

auto pTableView::setSelection(u32 selection) -> void {
  [[cocoaView content] selectRowIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(selection, 1)] byExtendingSelection:NO];
}
*/

}

#endif
