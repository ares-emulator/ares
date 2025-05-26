#if defined(Hiro_HexEdit)

@implementation CocoaHexEdit

-(id) initWith:(hiro::mHexEdit&)hexEditReference {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
    hexEdit = &hexEditReference;
    if (tableView = [[NSTableView alloc] initWithFrame:[self bounds]]) {
      [tableView setDataSource:self];
      // [tableView setHeaderView:nil];
      [tableView setUsesAlternatingRowBackgroundColors:true];
      [tableView setGridStyleMask:(NSTableViewDashedHorizontalGridLineMask | NSTableViewSolidVerticalGridLineMask)];
      [tableView setRowHeight:14.0];
      
      // remove padding and stuff to fit all 16 columns
      [tableView setIntercellSpacing:NSMakeSize(0, 0)];
      // [tableView setStyle:NSTableViewStylePlain];
      NSTableColumn* addressCol = [[NSTableColumn alloc] initWithIdentifier:@"Address"];
      addressCol.editable = NO;
      addressCol.title = @"  Address";
      [tableView addTableColumn:addressCol];
      addressCol.width = 60;
      
      for (int i = 0; i < hexEdit->columns(); i++) {
        NSTableColumn* col = [[NSTableColumn alloc] initWithIdentifier:[NSString stringWithFormat:@"%d",
                                                                        i]];
        col.title = [NSString stringWithFormat:@"0%x", i];
        col.width = 21;
        [tableView addTableColumn:col];
      }
      
      NSTableColumn* charCol = [[NSTableColumn alloc] initWithIdentifier:@"Char"];
      charCol.title = @"  Plain Text";
      charCol.editable = NO;
      [tableView addTableColumn:charCol];
      
      // must programmatically do this so the table shows up.
      [self setDocumentView:tableView];
      [self setHasVerticalScroller:YES];
      [self setHasHorizontalScroller:YES];
    }
  }
  return self;
}

- (NSTableView *) tableView {
  return tableView;
}

- (hiro::mHexEdit *) hexEdit {
  return hexEdit;
}


- (NSInteger) numberOfRowsInTableView:(NSTableView *)tableView {
  return (max(1, hexEdit->length()) + hexEdit->columns() - 1) / hexEdit->columns();
}

-(id) tableView:(NSTableView *) tableView
objectValueForTableColumn:(NSTableColumn *) tableColumn
            row:(NSInteger) row {
  // return a string with the content of (row, column)
  // u32 address = hexEdit->address() + row * hexEdit->columns();
  u32 address = row * hexEdit->columns();
  // address only
  if ([[tableColumn identifier] isEqualToString:@"Address"]) {
    return [NSString stringWithUTF8String:hex(address, 8L).data()];
  } else if ([[tableColumn identifier] isEqualToString:@"Char"]){
    // create the char string by concatenating
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
      switch(hexEdit->base()) {
        case 2: {
          NSMutableString *binary = [NSMutableString stringWithCapacity:8];
          for(int bit = 7; bit >= 0; bit--) {
            [binary appendFormat:@"%c", (data & (1 << bit)) ? '1' : '0'];
          }
          return binary;
        }
        case 8:
          return [NSString stringWithFormat:@"%03o", data];
        case 16:
        default:
          return [NSString stringWithFormat:@"%02X", data];
      }
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
  u32 address = row * hexEdit->columns() + colNumber;
  
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
  [tableView reloadData];
}

@end

namespace hiro {
  
  auto pHexEdit::construct() -> void {
    cocoaView = cocoaHexEdit = [[CocoaHexEdit alloc] initWith:self()];
    pWidget::construct();

    for (NSTableColumn *column in cocoaHexEdit.tableView.tableColumns) {
      if (@available(macOS 10.15, *)) {
        NSTextFieldCell *cell = column.dataCell;
        NSTableHeaderCell *headerCell = column.headerCell;
      
        // Set the font for the data cell.
        cell.font = [NSFont monospacedSystemFontOfSize:10 weight:NSFontWeightRegular];
      }
    }
    update();
  }
  
  auto pHexEdit::destruct() -> void {
    [cocoaView removeFromSuperview];
  }
  
  auto pHexEdit::setAddress(u32 offset) -> void {
    mHexEdit* hexEdit = [cocoaHexEdit hexEdit];
    int row = offset / hexEdit->columns();
    [[cocoaHexEdit tableView] scrollRowToVisible:row];
  }
  
  auto pHexEdit::setBackgroundColor(Color color) -> void {
    update();
  }

  auto pHexEdit::setBase(u16 base) -> void {
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
