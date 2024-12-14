#if defined(Hiro_HexEdit)

@implementation CocoaHexEdit

-(id) initWith:(hiro::mHexEdit&)hexEditReference {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
    hexEdit = &hexEditReference;
    if (tableView = [[NSTableView alloc] initWithFrame:[self bounds]]) {
      [tableView setDataSource:self];
      [tableView setHeaderView:nil];
      
      // remove padding and stuff to fit all 16 columns
      [tableView setIntercellSpacing:NSMakeSize(0, 0)];
      [tableView setStyle:NSTableViewStylePlain];
      NSTableColumn* addressCol = [[NSTableColumn alloc] initWithIdentifier:@"Address"];
      addressCol.editable = NO;
      [tableView addTableColumn:addressCol];
      
      for (int i = 0; i < hexEdit->columns(); i++) {
        NSTableColumn* col = [[NSTableColumn alloc] initWithIdentifier:[NSString stringWithFormat:@"%d",
                                                                        i]];
        col.width = 20;
        col.title = @"";
        [tableView addTableColumn:col];
      }
      /*
       NSTableColumn* hexCol = [[NSTableColumn alloc] initWithIdentifier:@"Hex"];
       [hexCol setTitle:@"Hex"];
       [tableView addTableColumn:hexCol];
       */
      
      NSTableColumn* ansiCol = [[NSTableColumn alloc] initWithIdentifier:@"ANSI"];
      ansiCol.editable = NO;
      [tableView addTableColumn:ansiCol];
      
      // must programmatically do this so the table shows up.
      [self setDocumentView:tableView];
      [self setHasVerticalScroller:YES];
    }
  }
  return self;
}

- (NSTableView *) tableView {
  return tableView;
}

- (NSInteger) numberOfRowsInTableView:(NSTableView *)tableView {
  return hexEdit->rows();
}

-(id) tableView:(NSTableView *) tableView
objectValueForTableColumn:(NSTableColumn *) tableColumn
            row:(NSInteger) row {
  // return a string with the content of (row, column)
  u32 address = hexEdit->address() + row * hexEdit->columns();
  // address only
  if ([[tableColumn identifier] isEqualToString:@"Address"]) {
    return [NSString stringWithUTF8String:hex(address, 8L).data()];
  } else if ([[tableColumn identifier] isEqualToString:@"ANSI"]){
    // create the ansi string by concatenating
    NSMutableString *output = [NSMutableString stringWithCapacity:hexEdit->columns()];
    for (auto column : range(hexEdit->columns())) {
      if (address < hexEdit->length()) {
        u8 data = hexEdit->doRead(address++);
        [output appendString:[NSString stringWithFormat:@"%c",
                              (data >= 0x20 && data <= 0x7e ? (char)data : '.')]];
      }
    }
    return output;
  } else {
    // return only the single byte at the correct position
    NSInteger columnNumber = [tableColumn.identifier integerValue];
    address += columnNumber;
    if (address < hexEdit->length()) {
      u8 data = hexEdit->doRead(address);
      return [NSString stringWithUTF8String:hex(data, 2L).data()];
    } else {
      return @"  ";
    }
  }
}

-(void) tableView:(NSTableView *) tableView
   setObjectValue:(id) object
   forTableColumn:(NSTableColumn *) tableColumn
              row:(NSInteger) row {
  // when table is edited, modify underlying data source
  NSInteger colNumber = [tableColumn.identifier integerValue];
  u32 address = hexEdit->address() + row * hexEdit->columns() + colNumber;
  
  // unclear if this is needed, but just to make sure the cast is safe
  if ([object isKindOfClass:[NSString class]]) {
    if (address < hexEdit->length()) {
      // only get the first 2 characters
      NSString* newVal = [(NSString *)object substringToIndex:2];
      NSScanner* hexScanner = [NSScanner scannerWithString:newVal];
      unsigned int data = 0;
      if ([hexScanner scanHexInt:&data]) {
        // this.......is probably ok....???
        hexEdit->doWrite(address, (u8)data);
      }
    }
    
  }
  [tableView reloadData];
}

@end

namespace hiro {
  
  auto pHexEdit::construct() -> void {
    cocoaView = cocoaHexEdit = [[CocoaHexEdit alloc] initWith:self()];
    pWidget::construct();
    for (NSTableColumn *column in cocoaHexEdit.tableView.tableColumns) {
      NSTextFieldCell *cell = column.dataCell;
      
      // Set the font for the data cell.
      cell.font = [NSFont monospacedSystemFontOfSize:10 weight:NSFontWeightRegular];
    }
    update();
  }
  
  auto pHexEdit::destruct() -> void {
    [cocoaView removeFromSuperview];
  }
  
  // these helper functions are unneeded, we can just reload the table data and
  // it will automatically reflect the contents of hexEdit!
  auto pHexEdit::setAddress(u32 offset) -> void {
    update();
  }
  
  auto pHexEdit::setBackgroundColor(Color color) -> void {
    update();
  }
  
  auto pHexEdit::setColumns(u32 columns) -> void {
    update();
  }
  
  auto pHexEdit::setForegroundColor(Color color) -> void {
    update();
  }
  
  auto pHexEdit::setLength(u32 length) -> void {
    update();
  }
  
  auto pHexEdit::setRows(u32 rows) -> void {
    update();
  }
  
  auto pHexEdit::update() -> void {
    [[cocoaHexEdit tableView] reloadData];
  }
  
}

#endif
