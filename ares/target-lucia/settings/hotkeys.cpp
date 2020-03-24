auto HotkeySettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  inputLabel.setText("Hotkey Bindings").setFont(Font().setBold());
  inputList.setBatchable();
  inputList.setHeadered();
  inputList.onChange([&] { eventChange(); });
  inputList.onActivate([&](auto cell) { eventAssign(); });

  reload();

  assignLabel.setFont(Font().setBold());
  spacer.setFocusable();
  assignButton.setText("Assign").onActivate([&] { eventAssign(); });
  clearButton.setText("Clear").onActivate([&] { eventClear(); });
}

auto HotkeySettings::reload() -> void {
  inputList.reset();
  inputList.append(TableViewColumn().setText("Name"));
  inputList.append(TableViewColumn().setText("Mapping").setExpandable());

  for(auto& mapping : inputManager.hotkeys) {
    TableViewItem item{&inputList};
    item.append(TableViewCell().setText(mapping.name).setFont(Font().setBold()));
    item.append(TableViewCell());
  }

  refresh();
  eventChange();
}

auto HotkeySettings::refresh() -> void {
  uint index = 0;
  for(auto& mapping : inputManager.hotkeys) {
    inputList.item(index++).cell(1).setIcon(mapping.icon()).setText(mapping.text());
  }
}

auto HotkeySettings::eventChange() -> void {
  assignButton.setEnabled(inputList.batched().size() == 1);
  clearButton.setEnabled(inputList.batched().size() >= 1);
}

auto HotkeySettings::eventClear() -> void {
  for(auto& item : inputList.batched()) {
    activeMappingBeginTimestamp = chrono::millisecond() + 50;
    activeMapping = inputManager.hotkeys[item.offset()];
    shared_pointer<HID::Device> device{new HID::Null};
    eventInput(device, 0, 0, 0, 0);
  }
}

auto HotkeySettings::eventAssign() -> void {
  if(ruby::input.driver() == "None") return (void)MessageDialog().setText(
    "Bindings cannot be set when no input driver has been loaded.\n"
    "Please go to driver settings and activate an input driver first."
  ).setAlignment(settingsWindow).error();

  if(auto item = inputList.selected()) {
    activeMappingBeginTimestamp = chrono::millisecond();
    activeMapping = inputManager.hotkeys[item.offset()];

    assignLabel.setText({"Press a key or button to assign to [", activeMapping->name, "] ..."});
    settingsWindow.setDismissable(false);
    Application::processEvents();
    spacer.setFocused();
  }
}

auto HotkeySettings::eventCancel() -> void {
  if(activeMapping) {
    activeMapping.reset();
    assignLabel.setText();
    settingsWindow.setDismissable(true);
  }
}

auto HotkeySettings::eventInput(shared_pointer<HID::Device> device, uint groupID, uint inputID, int16_t oldValue, int16_t newValue) -> void {
  if(!activeMapping) return;
  if(!settingsWindow.focused()) return;
  if(chrono::millisecond() - activeMappingBeginTimestamp < 50) return;

  if(activeMapping->bind(device, groupID, inputID, oldValue, newValue)) {
    eventCancel();
    refresh();
    inputList.setFocused();
  }
}
