#if defined(Hiro_ListView)

mListView::mListView() {
  mTableView::onActivate([&](auto) { doActivate(); });
  mTableView::onChange([&] { doChange(); });
  mTableView::onContext([&](auto cell) { doContext(); });
  mTableView::onToggle([&](TableViewCell cell) {
    if(auto item = cell->parentTableViewItem()) {
      if(auto shared = item->instance.acquire()) {
        doToggle(ListViewItem{shared});
      }
    }
  });
  append(TableViewColumn().setExpandable());
}

auto mListView::batched() const -> std::vector<ListViewItem> {
  auto batched = mTableView::batched();
  std::vector<ListViewItem> result;
  for(auto item : batched) result.push_back(ListViewItem{item});
  return result;
}

auto mListView::doActivate() const -> void {
  if(state.onActivate) state.onActivate();
}

auto mListView::doChange() const -> void {
  if(state.onChange) state.onChange();
}

auto mListView::doContext() const -> void {
  if(state.onContext) state.onContext();
}

auto mListView::doToggle(ListViewItem item) const -> void {
  if(state.onToggle) state.onToggle(item);
}

auto mListView::item(u32 position) const -> ListViewItem {
  return ListViewItem{mTableView::item(position)};
}

auto mListView::items() const -> std::vector<ListViewItem> {
  auto items = mTableView::items();
  std::vector<ListViewItem> result;
  for(auto item : items) result.push_back(ListViewItem{item});
  return result;
}

auto mListView::onActivate(const std::function<void ()>& callback) -> type& {
  state.onActivate = callback;
  return *this;
}

auto mListView::onChange(const std::function<void ()>& callback) -> type& {
  state.onChange = callback;
  return *this;
}

auto mListView::onContext(const std::function<void ()>& callback) -> type& {
  state.onContext = callback;
  return *this;
}

auto mListView::onToggle(const std::function<void (ListViewItem)>& callback) -> type& {
  state.onToggle = callback;
  return *this;
}

auto mListView::reset() -> type& {
  mTableView::reset();
  append(TableViewColumn().setExpandable());
  return *this;
}

auto mListView::resizeColumn() -> type& {
  mTableView::resizeColumns();
  return *this;
}

auto mListView::selected() const -> ListViewItem {
  return ListViewItem{mTableView::selected()};
}

auto mListView::setVisible(bool visible) -> type& {
  mTableView::setVisible(visible);
  return *this;
}

//

mListViewItem::mListViewItem() {
  append(TableViewCell());
}

auto mListViewItem::checkable() const -> bool {
  return cell(0).checkable();
}

auto mListViewItem::checked() const -> bool {
  return cell(0).checked();
}

auto mListViewItem::icon() const -> multiFactorImage {
  return cell(0).icon();
}

auto mListViewItem::reset() -> type& {
  mTableViewItem::reset();
  append(TableViewCell());
  return *this;
}

auto mListViewItem::setCheckable(bool checkable) -> type& {
  cell(0).setCheckable(checkable);
  return *this;
}

auto mListViewItem::setChecked(bool checked) -> type& {
  cell(0).setChecked(checked);
  return *this;
}

auto mListViewItem::setIcon(const multiFactorImage& icon) -> type& {
  cell(0).setIcon(icon);
  return *this;
}

auto mListViewItem::setText(const string& text) -> type& {
  cell(0).setText(text);
  return *this;
}

auto mListViewItem::text() const -> string {
  return cell(0).text();
}

#endif
