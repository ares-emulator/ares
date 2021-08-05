auto GraphicsViewer::construct() -> void {
  setCollapsible();
  setVisible(false);

  graphicsLabel.setText("Graphics Viewer").setFont(Font().setBold());
  graphicsList.onChange([&] { eventChange(); });
  graphicsView.setAlignment({0.0, 0.0});
  exportButton.setText("Export").onActivate([&] {
    eventExport();
  });
  liveOption.setText("Live");
  refreshButton.setText("Refresh").onActivate([&] {
    refresh();
  });
}

auto GraphicsViewer::reload() -> void {
  graphicsList.reset();
  for(auto graphics : ares::Node::enumerate<ares::Node::Debugger::Graphics>(emulator->root)) {
    ComboButtonItem item{&graphicsList};
    item.setAttribute<ares::Node::Debugger::Graphics>("node", graphics);
    item.setText(graphics->name());
  }
  eventChange();
}

auto GraphicsViewer::unload() -> void {
  graphicsList.reset();
  eventChange();
}

auto GraphicsViewer::refresh() -> void {
  if(auto item = graphicsList.selected()) {
    if(auto graphics = item.attribute<ares::Node::Debugger::Graphics>("node")) {
      auto width  = graphics->width();
      auto height = graphics->height();
      auto input  = graphics->capture();
      u32 offset = 0;
      image view;
      view.allocate(width, height);
      for(u32 y : range(height)) {
        auto output = view.data() + y * view.pitch();
        for(u32 x : range(width)) {
          view.write(output, 255 << 24 | input[offset++]);
          output += view.stride();
        }
      }
      graphicsView.setIcon(view);
    }
  } else {
    graphicsView.setIcon();
  }
}

auto GraphicsViewer::liveRefresh() -> void {
  if(visible() && liveOption.checked()) refresh();
}

auto GraphicsViewer::eventChange() -> void {
  refresh();
}

auto GraphicsViewer::eventExport() -> void {
  if(auto item = graphicsList.selected()) {
    if(auto graphics = item.attribute<ares::Node::Debugger::Graphics>("node")) {
      auto width  = graphics->width();
      auto height = graphics->height();
      auto input  = graphics->capture();
      auto identifier = graphics->name().downcase().replace(" ", "-");
      auto datetime = chrono::local::datetime().replace("-", "").replace(":", "").replace(" ", "-");
      auto location = emulator->locate({Location::notsuffix(emulator->game->location), "-", identifier, "-", datetime, ".png"}, ".png", settings.paths.debugging);
      Encode::PNG::RGB8(location, input.data(), width * sizeof(u32), width, height);
    }
  }
}

auto GraphicsViewer::setVisible(bool visible) -> GraphicsViewer& {
  if(visible) refresh();
  VerticalLayout::setVisible(visible);
  return *this;
}
