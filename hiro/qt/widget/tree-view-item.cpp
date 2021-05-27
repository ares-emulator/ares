#if defined(Hiro_TreeView)

namespace hiro {

auto pTreeViewItem::construct() -> void {
  qtStandardItem = new QStandardItem;
  qtStandardItem->setEditable(false);

  if(auto parentWidget = _parentWidget()) {
    auto parentItem = _parentItem();
    if(auto parentItem = _parentItem()) {
      parentItem->qtStandardItem->appendRow(qtStandardItem);
    } else {
      parentWidget->qtStandardItemModel->appendRow(qtStandardItem);
    }
  }

  setBackgroundColor(state().backgroundColor);
  setCheckable(state().checkable);
  setChecked(state().checked);
  setExpanded(state().expanded);
  setForegroundColor(state().foregroundColor);
  setIcon(state().icon);
  setText(state().text);
}

auto pTreeViewItem::destruct() -> void {
  delete qtStandardItem;
  qtStandardItem = nullptr;
}

//

auto pTreeViewItem::append(sTreeViewItem item) -> void {
}

auto pTreeViewItem::remove(sTreeViewItem item) -> void {
}

auto pTreeViewItem::setBackgroundColor(Color color) -> void {
  qtStandardItem->setBackground(CreateBrush(self().backgroundColor(true)));
}

auto pTreeViewItem::setCheckable(bool checkable) -> void {
  if(checkable) {
    qtStandardItem->setCheckState(state().checked? Qt::Checked : Qt::Unchecked);
  } else {
    qtStandardItem->setData(QVariant(), Qt::CheckStateRole);
  }
}

auto pTreeViewItem::setChecked(bool checked) -> void {
  if(state().checkable) {
    qtStandardItem->setCheckState(state().checked ? Qt::Checked : Qt::Unchecked);
  }
}

auto pTreeViewItem::setExpanded(bool expanded) -> void {
  if(auto parentWidget = _parentWidget()) {
    auto index = parentWidget->qtStandardItemModel->indexFromItem(qtStandardItem);
    if(expanded) {
      parentWidget->qtTreeView->expand(index);
    } else {
      parentWidget->qtTreeView->collapse(index);
    }
  }
}

auto pTreeViewItem::setFocused() -> void {
  //todo
}

//todo: not connected in TreeViewItem yet
auto pTreeViewItem::setFont(const string& font) -> void {
  qtStandardItem->setFont(pFont::create(self().font(true)));
}

auto pTreeViewItem::setForegroundColor(Color color) -> void {
  qtStandardItem->setForeground(CreateBrush(self().foregroundColor(true)));
}

auto pTreeViewItem::setIcon(const image& icon) -> void {
  qtStandardItem->setIcon(CreateIcon(state().icon));
}

auto pTreeViewItem::setSelected() -> void {
  if(auto parentWidget = _parentWidget()) {
    auto index = parentWidget->qtStandardItemModel->indexFromItem(qtStandardItem);
    auto selectionModel = parentWidget->qtTreeView->selectionModel();
    selectionModel->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }
}

auto pTreeViewItem::setText(const string& text) -> void {
  qtStandardItem->setText(QString::fromUtf8(text));
}

//

auto pTreeViewItem::_parentItem() -> pTreeViewItem* {
  if(auto parentItem = self().parentTreeViewItem()) return parentItem->self();
  return nullptr;
}

auto pTreeViewItem::_parentWidget() -> pTreeView* {
  if(auto parentWidget = self().parentTreeView(true)) return parentWidget->self();
  return nullptr;
}

}

#endif
