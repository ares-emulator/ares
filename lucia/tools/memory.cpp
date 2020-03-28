auto MemoryEditor::construct() -> void {
  setCollapsible();
  setVisible(false);

  memoryLabel.setText("Memory Editor").setFont(Font().setBold());
  memoryList.onChange([&] { eventChange(); });
  memoryEditor.setFont(Font().setFamily(Font::Mono));
  memoryEditor.setRows(24);

  exportButton.setText("Export").onActivate([&] {
    eventExport();
  });

  gotoLabel.setText("Goto:");
  gotoAddress.setFont(Font().setFamily(Font::Mono)).onActivate([&] {
    auto address = gotoAddress.text().hex();
    memoryEditor.setAddress(address);
    gotoAddress.setText();
  });

  liveOption.setText("Live");

  refreshButton.setText("Refresh").onActivate([&] {
    memoryEditor.update();
  });
}

auto MemoryEditor::reload() -> void {
  memoryList.reset();
  for(auto memory : ares::Node::enumerate<ares::Node::Memory>(emulator->root)) {
    ComboButtonItem item{&memoryList};
    item.setAttribute<ares::Node::Memory>("node", memory);
    item.setText(memory->name());
  }
  gotoAddress.setText();
  eventChange();
}

auto MemoryEditor::unload() -> void {
  memoryList.reset();
  gotoAddress.setText();
  eventChange();
}

auto MemoryEditor::refresh() -> void {
  memoryEditor.update();
}

auto MemoryEditor::liveRefresh() -> void {
  if(visible() && liveOption.checked()) refresh();
}

auto MemoryEditor::eventChange() -> void {
  if(auto item = memoryList.selected()) {
    if(auto memory = item.attribute<ares::Node::Memory>("node")) {
      memoryEditor.setLength(memory->size());
      memoryEditor.onRead([=](uint address) -> uint8_t {
        return memory->read(address);
      });
      memoryEditor.onWrite([=](uint address, uint8_t data) -> void {
        return memory->write(address, data);
      });
    }
  } else {
    memoryEditor.setLength(0);
    memoryEditor.onRead();
    memoryEditor.onWrite();
  }
  memoryEditor.setAddress(0);
  if(visible()) memoryEditor.update();
}

auto MemoryEditor::eventExport() -> void {
  if(auto item = memoryList.selected()) {
    if(auto memory = item.attribute<ares::Node::Memory>("node")) {
      auto identifier = memory->name().downcase().replace(" ", "-");
      auto datetime = chrono::local::datetime().replace("-", "").replace(":", "").replace(" ", "-");
      auto location = emulator->locate({Location::notsuffix(emulator->game.location), "-", identifier, "-", datetime, ".bin"}, ".bin", settings.paths.debugging);
      if(auto fp = file::open(location, file::mode::write)) {
        for(uint address : range(memory->size())) {
          fp.write(memory->read(address));
        }
      }
    }
  }
}

auto MemoryEditor::setVisible(bool visible) -> MemoryEditor& {
  if(visible) refresh();
  VerticalLayout::setVisible(visible);
  return *this;
}
