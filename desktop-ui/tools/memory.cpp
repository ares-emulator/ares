auto MemoryEditor::construct() -> void {
  setCollapsible();
  setVisible(false);

  memoryLabel.setText("Memory Editor").setFont(Font().setBold());
  memoryList.onChange([&] { eventChange(); });
  memoryEditor.setFont(Font().setFamily(Font::Mono));
  memoryEditor.setRows(24);

  exportButton.setText("Export").onActivate([&] {
    Program::Guard guard;
    eventExport();
  });

  gotoLabel.setText("Goto:");
  gotoAddress.setFont(Font().setFamily(Font::Mono)).onActivate([&] {
    Program::Guard guard;
    auto address = gotoAddress.text().hex();
    memoryEditor.setAddress(address);
    gotoAddress.setText();
  });

  liveOption.setText("Live");

  refreshButton.setText("Refresh").onActivate([&] {
    Program::Guard guard;
    refresh();
  });
}

auto MemoryEditor::reload() -> void {
  memoryList.reset();
  for(auto memory : ares::Node::enumerate<ares::Node::Debugger::Memory>(emulator->root)) {
    ComboButtonItem item{&memoryList};
    item.setAttribute<ares::Node::Debugger::Memory>("node", memory);
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
  if(auto item = memoryList.selected()) {
    if(auto memory = item.attribute<ares::Node::Debugger::Memory>("node")) {
      auto size = memory->size();
      if(previousData.size() != size) {
        //first refresh after selecting this memory node: seed the baseline, nothing to highlight yet
        previousData.resize(size);
        changedData.assign(size, false);
        for(u32 address : range(size)) previousData[address] = memory->read(address);
      } else {
        for(u32 address : range(size)) {
          u8 data = memory->read(address);
          changedData[address] = data != previousData[address];
          previousData[address] = data;
        }
      }
    }
  }
  memoryEditor.update();
}

auto MemoryEditor::liveRefresh() -> void {
  if(visible() && liveOption.checked()) refresh();
}

auto MemoryEditor::eventChange() -> void {
  if(auto item = memoryList.selected()) {
    if(auto memory = item.attribute<ares::Node::Debugger::Memory>("node")) {
      memoryEditor.setLength(memory->size());
      memoryEditor.onRead([=](u32 address) -> u8 {
        return memory->read(address);
      });
      memoryEditor.onWrite([=](u32 address, u8 data) -> void {
        Program::Guard guard;
        return memory->write(address, data);
      });
      memoryEditor.onHighlight([this](u32 address) -> bool {
        return address < changedData.size() && changedData[address];
      });
      previousData.clear();
      changedData.clear();
    }
  } else {
    memoryEditor.setLength(0);
    memoryEditor.onRead();
    memoryEditor.onWrite();
    memoryEditor.onHighlight();
    previousData.clear();
    changedData.clear();
  }
  memoryEditor.setAddress(0);
  if(visible()) memoryEditor.update();
}

auto MemoryEditor::eventExport() -> void {
  Program::Guard guard;
  if(auto item = memoryList.selected()) {
    if(auto memory = item.attribute<ares::Node::Debugger::Memory>("node")) {
      auto identifier = memory->name().downcase().replace(" ", "-");
      auto datetime = chrono::local::datetime().replace("-", "").replace(":", "").replace(" ", "-");
      auto location = emulator->locate({Location::notsuffix(emulator->game->location), "-", identifier, "-", datetime, ".bin"}, ".bin", settings.paths.debugging);
      if(auto fp = file::open(location, file::mode::write)) {
        for(u32 address : range(memory->size())) {
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
