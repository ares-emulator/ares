#if defined(Hiro_TableView)

auto mTableView::allocate() -> pObject* {
  return new pTableView(*this);
}

auto mTableView::destruct() -> void {
  for(auto& item : state.items) item->destruct();
  for(auto& column : state.columns) column->destruct();
  mWidget::destruct();
}

//

auto mTableView::alignment() const -> Alignment {
  return state.alignment;
}

auto mTableView::append(sTableViewColumn column) -> type& {
  state.columns.push_back(column);
  column->setParent(this, columnCount() - 1);
  signal(append, column);
  return *this;
}

auto mTableView::append(sTableViewItem item) -> type& {
  state.items.push_back(item);
  item->setParent(this, itemCount() - 1);
  signal(append, item);
  return *this;
}

auto mTableView::backgroundColor() const -> Color {
  return state.backgroundColor;
}

auto mTableView::batchable() const -> bool {
  return state.batchable;
}

auto mTableView::batched() const -> std::vector<TableViewItem> {
  std::vector<TableViewItem> items;
  for(auto& item : state.items) {
    if(item->selected()) items.push_back(item);
  }
  return items;
}

auto mTableView::bordered() const -> bool {
  return state.bordered;
}

auto mTableView::column(u32 position) const -> TableViewColumn {
  if(position < columnCount()) return state.columns[position];
  return {};
}

auto mTableView::columnCount() const -> u32 {
  return state.columns.size();
}

auto mTableView::columns() const -> std::vector<TableViewColumn> {
  std::vector<TableViewColumn> result;
  for(auto& column : state.columns) result.push_back(column);
  return result;
}

auto mTableView::doActivate(sTableViewCell cell) const -> void {
  if(state.onActivate) return state.onActivate(cell);
}

auto mTableView::doChange() const -> void {
  if(state.onChange) return state.onChange();
}

auto mTableView::doContext(sTableViewCell cell) const -> void {
  if(state.onContext) return state.onContext(cell);
}

auto mTableView::doEdit(sTableViewCell cell) const -> void {
  if(state.onEdit) return state.onEdit(cell);
}

auto mTableView::doSort(sTableViewColumn column) const -> void {
  if(state.onSort) return state.onSort(column);
}

auto mTableView::doToggle(sTableViewCell cell) const -> void {
  if(state.onToggle) return state.onToggle(cell);
}

auto mTableView::foregroundColor() const -> Color {
  return state.foregroundColor;
}

auto mTableView::headered() const -> bool {
  return state.headered;
}

auto mTableView::item(u32 position) const -> TableViewItem {
  if(position < itemCount()) return state.items[position];
  return {};
}

auto mTableView::itemCount() const -> u32 {
  return state.items.size();
}

auto mTableView::items() const -> std::vector<TableViewItem> {
  std::vector<TableViewItem> items;
  for(auto& item : state.items) items.push_back(item);
  return items;
}

auto mTableView::onActivate(const std::function<void (TableViewCell)>& callback) -> type& {
  state.onActivate = callback;
  return *this;
}

auto mTableView::onChange(const std::function<void ()>& callback) -> type& {
  state.onChange = callback;
  return *this;
}

auto mTableView::onContext(const std::function<void (TableViewCell)>& callback) -> type& {
  state.onContext = callback;
  return *this;
}

auto mTableView::onEdit(const std::function<void (TableViewCell)>& callback) -> type& {
  state.onEdit = callback;
  return *this;
}

auto mTableView::onSort(const std::function<void (TableViewColumn)>& callback) -> type& {
  state.onSort = callback;
  return *this;
}

auto mTableView::onToggle(const std::function<void (TableViewCell)>& callback) -> type& {
  state.onToggle = callback;
  return *this;
}

auto mTableView::remove(sTableViewColumn column) -> type& {
  signal(remove, column);
  state.columns.erase(state.columns.begin() + column->offset());
  for(u32 n : range(column->offset(), columnCount())) {
    state.columns[n]->adjustOffset(-1);
  }
  column->setParent();
  return *this;
}

auto mTableView::remove(sTableViewItem item) -> type& {
  signal(remove, item);
  state.items.erase(state.items.begin() + item->offset());
  for(u32 n : range(item->offset(), itemCount())) {
    state.items[n]->adjustOffset(-1);
  }
  item->setParent();
  return *this;
}

auto mTableView::reset() -> type& {
  while(!state.items.empty()) remove(state.items.back());
  while(!state.columns.empty()) remove(state.columns.back());
  return *this;
}

auto mTableView::resizeColumns() -> type& {
  signal(resizeColumns);
  return *this;
}

auto mTableView::selectAll() -> type& {
  if(!state.batchable) return *this;
  for(auto& item : state.items) {
    item->setSelected(true);
  }
  return *this;
}

auto mTableView::selectNone() -> type& {
  for(auto& item : state.items) {
    item->setSelected(false);
  }
  return *this;
}

auto mTableView::selected() const -> TableViewItem {
  for(auto& item : state.items) {
    if(item->selected()) return item;
  }
  return {};
}

auto mTableView::setAlignment(Alignment alignment) -> type& {
  state.alignment = alignment;
  signal(setAlignment, alignment);
  return *this;
}

auto mTableView::setBackgroundColor(Color color) -> type& {
  state.backgroundColor = color;
  signal(setBackgroundColor, color);
  return *this;
}

auto mTableView::setBatchable(bool batchable) -> type& {
  state.batchable = batchable;
  signal(setBatchable, batchable);
  return *this;
}

auto mTableView::setBordered(bool bordered) -> type& {
  state.bordered = bordered;
  signal(setBordered, bordered);
  return *this;
}

auto mTableView::setForegroundColor(Color color) -> type& {
  state.foregroundColor = color;
  signal(setForegroundColor, color);
  return *this;
}

auto mTableView::setHeadered(bool headered) -> type& {
  state.headered = headered;
  signal(setHeadered, headered);
  return *this;
}

auto mTableView::setParent(mObject* parent, s32 offset) -> type& {
  for(auto& item : state.items | std::views::reverse) item->destruct();
  for(auto& column : state.columns | std::views::reverse) column->destruct();
  mObject::setParent(parent, offset);
  for(auto& column : state.columns) column->setParent(this, column->offset());
  for(auto& item : state.items) item->setParent(this, item->offset());
  return *this;
}

auto mTableView::setUsesSidebarStyle(bool usesSidebarStyle) -> type& {
  #if defined(PLATFORM_MACOS)
  signal(setUsesSidebarStyle, usesSidebarStyle);
  #endif
  return *this;
}

auto mTableView::setSortable(bool sortable) -> type& {
    state.sortable = sortable;
    signal(setSortable, sortable);
    return *this;
}

auto mTableView::sort() -> type& {
  Sort sorting = Sort::None;
  u32 offset = 0;
  for(auto& column : state.columns) {
    if(column->sorting() == Sort::None) continue;
    sorting = column->sorting();
    offset = column->offset();
    break;
  }
  auto &itemsRef = state.items;
  std::ranges::stable_sort(itemsRef, [&](const sTableViewItem& lhs, const sTableViewItem& rhs) {
    string x = offset < lhs->cellCount() ? lhs->state.cells[offset]->state.text : ""_s;
    string y = offset < rhs->cellCount() ? rhs->state.cells[offset]->state.text : ""_s;
    if(sorting == Sort::Ascending ) return string::icompare(x, y) < 0;
    if(sorting == Sort::Descending) return string::icompare(y, x) < 0;
    return false;
  });
  return *this;
}

auto mTableView::sortable() const -> bool {
  return state.sortable;
}

#endif
