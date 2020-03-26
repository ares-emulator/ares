auto FirmwareSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  firmwareLabel.setText("BIOS Firmware Locations").setFont(Font().setBold());
  firmwareList.setHeadered();
  firmwareList.setBatchable();
  firmwareList.onChange([&] { eventChange(); });
  firmwareList.onActivate([&](auto) { eventAssign(); });
  assignButton.setText("Assign").onActivate([&] { eventAssign(); });
  clearButton.setText("Clear").onActivate([&] { eventClear(); });

  refresh();
}

auto FirmwareSettings::refresh() -> void {
  firmwareList.reset();
  firmwareList.append(TableViewColumn().setText("Emulator"));
  firmwareList.append(TableViewColumn().setText("Type").setVisible(false));
  firmwareList.append(TableViewColumn().setText("Region"));
  firmwareList.append(TableViewColumn().setText("Location").setExpandable());

  for(auto& emulator : emulators) {
    for(auto& firmware : emulator->firmware) {
      TableViewItem item{&firmwareList};
      item.append(TableViewCell().setText(emulator->name).setFont(Font().setBold()));
      item.append(TableViewCell().setText(firmware.type));
      item.append(TableViewCell().setText(firmware.region));
      if(file::exists(firmware.location)) {
        item.append(TableViewCell().setText(string{firmware.location}.replace(Path::user(), "~/")));
      } else {
        item.append(TableViewCell().setText("(unset)").setForegroundColor({128, 128, 128}));
      }
    }
  }

  firmwareList.resizeColumns();
  eventChange();
}

auto FirmwareSettings::select(const string& emulator, const string& type, const string& region) -> bool {
  for(auto& item : firmwareList.items()) {
    if(item.cell(0).text() != emulator) continue;
    if(item.cell(1).text() != type) continue;
    if(item.cell(2).text() != region) continue;
    firmwareList.selectNone();
    item.setSelected();
    eventChange();
    firmwareList.setFocused();
    return true;
  }
  return false;
}

auto FirmwareSettings::eventChange() -> void {
  auto batched = firmwareList.batched();
  assignButton.setEnabled(batched.size() == 1);
  clearButton.setEnabled(batched.size() >= 1);
}

auto FirmwareSettings::eventAssign() -> void {
  auto batched = firmwareList.batched();
  if(batched.size() != 1) return;
  auto& item = batched[0];
  auto name = item.cell(0).text();
  auto type = item.cell(1).text();
  auto region = item.cell(2).text();
  for(auto& emulator : emulators) {
    if(emulator->name != name) continue;
    for(auto& firmware : emulator->firmware) {
      if(firmware.type != type) continue;
      if(firmware.region != region) continue;
      BrowserDialog dialog;
      dialog.setTitle({"Select ", name, " ", type, " (", region, ")"});
      dialog.setPath(Path::desktop());
      if(auto location = program.openFile(dialog)) {
        firmware.location = location;
        refresh();
      }
    }
  }
}

auto FirmwareSettings::eventClear() -> void {
  for(auto& item : firmwareList.batched()) {
    auto name = item.cell(0).text();
    auto type = item.cell(1).text();
    auto region = item.cell(2).text();
    for(auto& emulator : emulators) {
      if(emulator->name != name) continue;
      for(auto& firmware : emulator->firmware) {
        if(firmware.type != type) continue;
        if(firmware.region != region) continue;
        firmware.location = "";
      }
    }
  }
  refresh();
}
