HotkeySettings::HotkeySettings(View* parent) : PanelItem(parent, Size{~0, ~0}) {
  setCollapsible().setVisible(false);

  headerLabel.setText("Hotkeys").setFont(Font().setBold());
  hotkeyList.setBatchable();
  hotkeyList.onActivate([&](auto cell) { eventAssign(); });
  hotkeyList.onChange([&] { eventChange(); });
  assignButton.setText("Assign").onActivate([&] { eventAssign(); });
  clearButton.setText("Clear").onActivate([&] { eventClear(); });
}

auto HotkeySettings::show() -> void {
  refresh();
  setVisible(true);
}

auto HotkeySettings::hide() -> void {
  setVisible(false);
  assigning.reset();
}

auto HotkeySettings::refresh() -> void {
  hotkeyList.reset().setEnabled();
  hotkeyList.append(TableViewColumn().setText("Name"));
  hotkeyList.append(TableViewColumn().setText("Mapping").setExpandable());
  for(auto& hotkey : hotkeys.hotkeys) {
    TableViewItem item{&hotkeyList};
    item.setAttribute<InputHotkey*>("hotkey", hotkey);
    TableViewCell name{&item};
    name.setText(hotkey->name).setFont(Font().setBold());
    TableViewCell value{&item};
  }
  update();
  hotkeyList.doChange();
}

auto HotkeySettings::update() -> void {
  for(auto& item : hotkeyList.items()) {
    auto value = item.cell(1);
    auto hotkey = item.attribute<InputHotkey*>("hotkey");
    if(hotkey->device) {
      auto device = hotkey->device->name();
      if(device == "Keyboard") value.setIcon(Icon::Device::Keyboard);
      else if(device == "Mouse") value.setIcon(Icon::Device::Mouse);
      else if(device == "Joypad") value.setIcon(Icon::Device::Joypad);
      else value.setIcon(Icon::Action::Close);
      string label;
      if(hotkey->device->isJoypad()) label.append(hotkey->device->group(hotkey->groupID).name(), ".");
      label.append(hotkey->device->group(hotkey->groupID).input(hotkey->inputID).name());
      value.setText(label);
    } else {
      value.setIcon(Icon::Action::Close);
      value.setText("(unmapped)");
    }
  }

  hotkeyList.resizeColumns();
}

auto HotkeySettings::eventAssign() -> void {
  inputManager.poll();
  assigningQueue = hotkeyList.batched();
  eventAssignNext();
}

auto HotkeySettings::eventAssignNext() -> void {
  if(!assigningQueue) {
    hotkeyList.selectNone().setFocused();
    return eventChange();
  }

  //use the viewport to sink inputs away from the table view during assignment
  program.viewport.setFocused();
  auto item = assigningQueue.takeFirst();
  hotkeyList.selectNone();
  item.setSelected().setFocused();
  auto hotkey = item.attribute<InputHotkey*>("hotkey");
  item.cell(1).setIcon(Icon::Go::Right).setText("(assign)");
  assigning = *hotkey;
  eventChange();
}

auto HotkeySettings::eventClear() -> void {
  if(auto batched = hotkeyList.batched()) {
    for(auto& item : batched) {
      auto hotkey = item.attribute<InputHotkey*>("hotkey");
      hotkey->identifier = {};
    }
    hotkeys.bind();
    update();
  }
}

auto HotkeySettings::eventChange() -> void {
  auto batched = hotkeyList.batched();
  assignButton.setEnabled((bool)batched);
  clearButton.setEnabled((bool)batched);
}

auto HotkeySettings::eventInput(shared_pointer<HID::Device> device, uint group, uint input, int16_t oldValue, int16_t newValue) -> void {
  if(!assigning) return;

  bool allow = false;

  if(device->isKeyboard()) {
    if(oldValue == 0 && newValue == 1) allow = true;
  }
  if(device->isJoypad() && group == HID::Joypad::Button) {
    if(oldValue == 0 && newValue == 1) allow = true;
  }

  if(!allow) return;

  assigning->identifier = {};
  if(device->group(group).input(input).name() != "Escape") {
    assigning->identifier.append(hex(device->pathID()), "/");
    assigning->identifier.append(hex(device->vendorID()), "/");
    assigning->identifier.append(hex(device->productID()), "/");
    assigning->identifier.append(device->group(group).name(), "/");
    assigning->identifier.append(device->group(group).input(input).name());
  }
  assigning.reset();
  hotkeys.bind();
  update();
  eventAssignNext();
}
