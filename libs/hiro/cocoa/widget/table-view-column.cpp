#if defined(Hiro_TableView)

namespace hiro {

auto pTableViewColumn::construct() -> void {
  if(auto tableView = _parent()) {
    [(CocoaTableView*)(tableView->cocoaView) reloadColumns];
  }
}

auto pTableViewColumn::destruct() -> void {
  if(auto tableView = _parent()) {
    [(CocoaTableView*)(tableView->cocoaView) reloadColumns];
  }
}

auto pTableViewColumn::setActive() -> void {
}

auto pTableViewColumn::setAlignment(Alignment alignment) -> void {
}

auto pTableViewColumn::setBackgroundColor(Color color) -> void {
}

auto pTableViewColumn::setEditable(bool editable) -> void {
}

auto pTableViewColumn::setExpandable(bool expandable) -> void {
}

auto pTableViewColumn::setFont(const Font& font) -> void {
}

auto pTableViewColumn::setForegroundColor(Color color) -> void {
}

auto pTableViewColumn::setHorizontalAlignment(double alignment) -> void {
}

auto pTableViewColumn::setIcon(const image& icon) -> void {
  //TODO
}

auto pTableViewColumn::setResizable(bool resizable) -> void {
}

auto pTableViewColumn::setSorting(Sort sorting) -> void {
  setText(state().text);
}

auto pTableViewColumn::setText(const string& text) -> void {
  if(auto parent = _parent()) {
    string label = text;
    if(state().sorting == Sort::Ascending ) label.append(" \u25b4");
    if(state().sorting == Sort::Descending) label.append(" \u25be");
    NSTableColumn* tableColumn = [[(CocoaTableView*)(parent->cocoaView) content] tableColumnWithIdentifier:[[NSNumber numberWithInteger:self().offset()] stringValue]];
    [[tableColumn headerCell] setStringValue:[NSString stringWithUTF8String:label]];
    [[[(CocoaTableView*)(parent->cocoaView) content] headerView] setNeedsDisplay:YES];
  }
}

auto pTableViewColumn::setVerticalAlignment(double alignment) -> void {
}

auto pTableViewColumn::setVisible(bool visible) -> void {
}

auto pTableViewColumn::setWidth(signed width) -> void {
  if(auto parent = _parent()) {
    NSTableColumn* tableColumn = [[(CocoaTableView*)(parent->cocoaView) content] tableColumnWithIdentifier:[[NSNumber numberWithInteger:self().offset()] stringValue]];
    tableColumn.width = width;
  }
}

auto pTableViewColumn::_parent() -> maybe<pTableView&> {
  if(auto parent = self().parentTableView()) {
    if(auto self = parent->self()) return *self;
  }
  return {};
}

}

#endif
