EventManager::EventManager(View* view) : PanelList(view, Size{~0, ~0}) {
  setCollapsible().setVisible(false);
  listView.onChange([&] { onChange(); });
  listView.onToggle([&](auto item) { onToggle(item); });
}

auto EventManager::show() -> void {
  listView.selectNone().doChange();
  refresh();
  setVisible(true);
}

auto EventManager::hide() -> void {
  setVisible(false);
}

auto EventManager::refresh() -> void {
  listView.reset();
  if(auto root = emulator.root) {
    for(auto& node : *root) refresh(node);
  }
}

auto EventManager::refresh(ares::Node::Object node) -> void {
  if(auto event = node->cast<ares::Node::Event>()) {
    ListViewItem item{&listView};
    item.setAttribute<ares::Node::Event>("event", event);
    item.setCheckable();
    item.setChecked(event->enabled());
    item.setText({event->component(), " ", event->name()});
  }

  for(auto& node : *node) refresh(node);
}

auto EventManager::onChange() -> void {
}

auto EventManager::onToggle(ListViewItem item) -> void {
  if(auto event = item.attribute<ares::Node::Event>("event")) {
    bool enabled = event->enabled();
    event->setEnabled(!enabled);
  }
}
