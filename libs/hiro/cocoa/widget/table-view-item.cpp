#if defined(Hiro_TableView)

namespace hiro {

auto pTableViewItem::construct() -> void {
}

auto pTableViewItem::destruct() -> void {
}

auto pTableViewItem::append(sTableViewCell cell) -> void {
  if(auto tableView = _parent()) {
    [[(CocoaTableView*)(tableView->cocoaView) content] reloadData];
  }
}

auto pTableViewItem::remove(sTableViewCell cell) -> void {
  if(auto tableView = _parent()) {
    [[(CocoaTableView*)(tableView->cocoaView) content] reloadData];
  }
}

auto pTableViewItem::setAlignment(Alignment alignment) -> void {
}

auto pTableViewItem::setBackgroundColor(Color color) -> void {
}

auto pTableViewItem::setFocused() -> void {
}

auto pTableViewItem::setForegroundColor(Color color) -> void {
}

auto pTableViewItem::setSelected(bool selected) -> void {
  if(auto tableView = _parent()) {
    auto lock = tableView->acquire();
    auto indexSet = [[NSMutableIndexSet alloc] init];
    for(auto& item : tableView->state().items) {
      if(item->selected()) [indexSet addIndex:item->offset()];
    }
    [[(CocoaTableView*)(tableView->cocoaView) content] selectRowIndexes:indexSet byExtendingSelection:NO];
  }
}

auto pTableViewItem::_parent() -> maybe<pTableView&> {
  if(auto parent = self().parentTableView()) {
    if(auto self = parent->self()) return *self;
  }
  return nothing;
}

}

#endif
