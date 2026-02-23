#if defined(Hiro_TreeView)

namespace hiro {

auto pTreeView::construct() -> void {
  qtWidget = qtTreeView = new QtTreeView(*this);
  qtStandardItemModel = new QStandardItemModel;
  qtTreeView->setHeaderHidden(true);
  qtTreeView->setModel(qtStandardItemModel);
  qtTreeView->setUniformRowHeights(true);

  setActivation(state().activation);
  setBackgroundColor(state().backgroundColor);
  setForegroundColor(state().foregroundColor);
  pWidget::construct();
}

auto pTreeView::destruct() -> void {
if(Application::state().quit) return;  //TODO: hack
  delete qtTreeView;
  delete qtStandardItemModel;
  qtWidget = qtTreeView = nullptr;
  qtStandardItemModel = nullptr;
}

//

auto pTreeView::append(sTreeViewItem item) -> void {
}

auto pTreeView::remove(sTreeViewItem item) -> void {
}

auto pTreeView::setActivation(Mouse::Click activation) -> void {
}

auto pTreeView::setBackgroundColor(Color color) -> void {
  static auto defaultColor = qtTreeView->palette().color(QPalette::Base);

  auto palette = qtTreeView->palette();
  palette.setColor(QPalette::Base, CreateColor(color, defaultColor));
  qtTreeView->setPalette(palette);
  qtTreeView->setAutoFillBackground((bool)color);
}

auto pTreeView::setForegroundColor(Color color) -> void {
  static auto defaultColor = qtTreeView->palette().color(QPalette::Text);

  auto palette = qtTreeView->palette();
  palette.setColor(QPalette::Text, CreateColor(color, defaultColor));
  qtTreeView->setPalette(palette);
}

}

#endif
