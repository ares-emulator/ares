auto ManifestViewer::construct() -> void {
  setCollapsible();
  setVisible(false);

  manifestLabel.setText("Manifest Viewer").setFont(Font().setBold());
  manifestList.onChange([&] { eventChange(); });
  manifestView.setEditable(false).setFont(Font().setFamily(Font::Mono));
}

auto ManifestViewer::reload() -> void {
  manifestList.reset();
  for(auto node : ares::Node::enumerate<ares::Node::Object>(emulator->root)) {
    if(auto pak = node->pak()) {
      if(auto fp = pak->read("manifest.bml")) {
        ComboButtonItem item{&manifestList};
        item.setAttribute<ares::Node::Object>("node", node);
        item.setText(node->name());
      }
    }
  }
  eventChange();
}

auto ManifestViewer::unload() -> void {
  manifestList.reset();
  eventChange();
}

auto ManifestViewer::refresh() -> void {
  if(auto item = manifestList.selected()) {
    if(auto node = item.attribute<ares::Node::Object>("node")) {
      if(auto pak = node->pak()) {
        if(auto fp = pak->read("manifest.bml")) {
          manifestView.setText(fp->reads());
        }
      }
    }
  } else {
    manifestView.setText();
  }
}

auto ManifestViewer::eventChange() -> void {
  refresh();
}

auto ManifestViewer::setVisible(bool visible) -> ManifestViewer& {
  if(visible) refresh();
  VerticalLayout::setVisible(visible);
  return *this;
}
