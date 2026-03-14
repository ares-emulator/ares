static auto finishAssignment(HotkeySettings& self) -> void {
  self.chordTimer.setEnabled(false);
  self.activeMapping.reset();
  self.pendingKeyboardChord.clear();
  self.assignLabel.setText();
  self.refresh();
  self.timer.onActivate([&] {
    self.timer.setEnabled(false);
    self.inputList.setFocused();
    settingsWindow.setDismissable(true);
  }).setInterval(200).setEnabled();
}

static auto keyboardName(std::shared_ptr<HID::Device> device, u32 groupID, u32 inputID) -> string {
  if(!device) return {};
  if(groupID >= device->size()) return {};
  if(inputID >= device->group(groupID).size()) return {};
  return device->group(groupID).input(inputID).name();
}

static auto keyboardChordText(const std::vector<HotkeySettings::ChordKey>& chord) -> string {
  string output;
  for(auto index : range(chord.size())) {
    auto name = keyboardName(chord[index].device, chord[index].groupID, chord[index].inputID);
    if(!name) continue;
    if(index) output.append("+");
    output.append(name);
  }
  return output;
}

static auto keyboardChordAssignment(const std::vector<HotkeySettings::ChordKey>& chord) -> string {
  if(chord.empty()) return {};

  string output;
  for(auto index : range(chord.size())) {
    auto& key = chord[index];
    if(index) output.append("+");
    output.append(string{"0x", hex(key.deviceID), "/", key.groupID, "/", key.inputID});
  }
  return output;
}

static auto keyboardChordContains(const std::vector<HotkeySettings::ChordKey>& chord, u32 groupID, u32 inputID) -> bool {
  for(auto& key : chord) {
    if(key.groupID == groupID && key.inputID == inputID) return true;
  }
  return false;
}

static auto hotkeyAssignmentText(string_view assignment) -> string {
  if(!assignment) return {};

  string output;
  auto parts = nall::split(assignment, "+");
  for(auto index : range(parts.size())) {
    auto token = nall::split(parts[index].strip(), "/");
    if(token.size() < 3) return {};

    u64 deviceID = token[0].natural();
    u32 groupID = token[1].natural();
    u32 inputID = token[2].natural();

    std::shared_ptr<HID::Device> device;
    for(auto& candidate : inputManager.devices) {
      if(candidate->id() == deviceID) {
        device = candidate;
        break;
      }
    }
    if(!device) return "(disconnected)";
    if(groupID >= device->size()) return {};
    if(inputID >= device->group(groupID).size()) return {};

    if(index) output.append("+");
    output.append(device->group(groupID).input(inputID).name());
  }

  return output;
}

auto HotkeySettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  inputList.setBatchable();
  inputList.setHeadered();
  inputList.onChange([&] { eventChange(); });
  inputList.onActivate([&](auto cell) { eventAssign(cell); });

  reload();

  assignLabel.setFont(Font().setBold());
  spacer.setFocusable();
  newButton.setText("New").onActivate([&] { eventNew(); });
  assignButton.setText("Assign").onActivate([&] { eventAssign(inputList.selected().cell(0)); });
  clearButton.setText("Clear").onActivate([&] { eventClear(); });
}

auto HotkeySettings::reload() -> void {
  inputList.reset();
  inputList.append(TableViewColumn().setText("Name"));
  for(u32 binding : range(BindingLimit)) {
    inputList.append(TableViewColumn().setText({"Mapping #", 1 + binding}).setExpandable());
  }

  for(auto& mapping : inputManager.hotkeys) {
    TableViewItem item{&inputList};
    item.append(TableViewCell().setText(mapping.name).setFont(Font().setBold()));
    for(u32 binding : range(BindingLimit)) item.append(TableViewCell());
  }

  refresh();
  eventChange();
  inputList.resizeColumns();
}

auto HotkeySettings::refresh() -> void {
  u32 index = 0;
  for(auto& mapping : inputManager.hotkeys) {
    for(u32 binding : range(BindingLimit)) {
      //do not remove identifier from mappings currently being assigned
      if(activeMapping && &activeMapping() == &mapping && activeBinding == binding) continue;
      auto cell = inputList.item(index).cell(1 + binding);
      auto assignment = mapping.assignments[binding];
      if(nall::split(assignment, "+").size() > 1) {
        cell.setIcon(Icon::Device::Keyboard);
        cell.setText(hotkeyAssignmentText(assignment));
      } else {
        cell.setIcon(mapping.bindings[binding].icon());
        cell.setText(mapping.bindings[binding].text());
      }
    }
    index++;
  }
}

auto HotkeySettings::eventChange() -> void {
  Program::Guard guard;
  newButton.setEnabled(newMenu.actionCount() || [&] {
    for(u32 slot : range(1, 10)) {
      if(!inputManager.hasCustomHotkey(InputHotkey::CustomType::SaveStateSlot, slot)) return true;
      if(!inputManager.hasCustomHotkey(InputHotkey::CustomType::LoadStateSlot, slot)) return true;
    }
    return false;
  }());
  assignButton.setEnabled(inputList.batched().size() == 1);
  clearButton.setEnabled(inputList.batched().size() >= 1);
}

auto HotkeySettings::eventNew() -> void {
  newMenu.setVisible(false);
  newMenu.reset();

  Menu saveStateMenu{&newMenu};
  saveStateMenu.setText("Save State");
  for(u32 slot : range(1, 10)) {
    if(inputManager.hasCustomHotkey(InputHotkey::CustomType::SaveStateSlot, slot)) continue;
    MenuItem item{&saveStateMenu};
    item.setText({"Slot ", slot}).onActivate([this, slot] {
      eventNew(InputHotkey::CustomType::SaveStateSlot, slot);
    });
  }
  if(!saveStateMenu.actionCount()) newMenu.remove(saveStateMenu);

  Menu loadStateMenu{&newMenu};
  loadStateMenu.setText("Load State");
  for(u32 slot : range(1, 10)) {
    if(inputManager.hasCustomHotkey(InputHotkey::CustomType::LoadStateSlot, slot)) continue;
    MenuItem item{&loadStateMenu};
    item.setText({"Slot ", slot}).onActivate([this, slot] {
      eventNew(InputHotkey::CustomType::LoadStateSlot, slot);
    });
  }
  if(!loadStateMenu.actionCount()) newMenu.remove(loadStateMenu);

  if(newMenu.actionCount()) newMenu.setVisible(true);
  eventChange();
}

auto HotkeySettings::eventNew(InputHotkey::CustomType type, u32 slot) -> void {
  if(!inputManager.appendCustomHotkey(type, slot)) return;

  reload();

  for(u32 index : range(inputManager.hotkeys.size())) {
    auto& mapping = inputManager.hotkeys[index];
    if(mapping.customType != type || mapping.customSlot != slot) continue;
    inputList.item(index).setSelected();
    inputList.setFocused();
    break;
  }
  eventChange();
}

auto HotkeySettings::eventClear() -> void {
  for(auto& item : inputList.batched()) {
    auto& mapping = inputManager.hotkeys[item.offset()];
    mapping.unbind();
  }
  refresh();
}

auto HotkeySettings::eventAssign(TableViewCell cell) -> void {
  inputManager.poll(true);  //clear any pending events first

  if(ruby::input.driver() == "None") return (void)MessageDialog().setText(
    "Bindings cannot be set when no input driver has been loaded.\n"
    "Please go to driver settings and activate an input driver first."
  ).setAlignment(settingsWindow).error();

  if(auto item = inputList.selected()) {
    if(activeMapping) refresh();  //clear any previous assign arrow prompts
    chordTimer.setEnabled(false);
    activeMapping = inputManager.hotkeys[item.offset()];
    activeBinding = max(0, (s32)cell.offset() - 1);
    pendingKeyboardChord.clear();

    item.cell(1 + activeBinding).setIcon(Icon::Go::Right).setText("(assign ...)");
    assignLabel.setText({"Press a key or button for mapping #", 1 + activeBinding, " [", activeMapping->name, "] ..."});
    refresh();
    settingsWindow.setDismissable(false);
    Application::processEvents();
    spacer.setFocused();
  }
}

auto HotkeySettings::eventInput(std::shared_ptr<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> void {
  if(!activeMapping) return;
  if(!settingsWindow.focused()) return;
  if(device->isMouse()) return;

  auto commitPendingChord = [&] {
    chordTimer.setEnabled(false);
    if(!activeMapping || pendingKeyboardChord.empty()) return;
    activeMapping->bind(activeBinding, keyboardChordAssignment(pendingKeyboardChord));
    finishAssignment(*this);
  };
  auto armChordCommit = [&] {
    chordTimer.onActivate(commitPendingChord).setInterval(1200).setEnabled();
  };

  if(device->isKeyboard()) {
    if(groupID != HID::Keyboard::GroupID::Button) return;

    auto name = keyboardName(device, groupID, inputID);
    if(name == "Escape" && oldValue == 0 && newValue != 0) {
      activeMapping->unbind(activeBinding);
      finishAssignment(*this);
      return;
    }

    if(oldValue == 0 && newValue != 0) {
      if(!pendingKeyboardChord.empty() && pendingKeyboardChord[0].deviceID != device->id()) return;

      if(!keyboardChordContains(pendingKeyboardChord, groupID, inputID)) {
        pendingKeyboardChord.push_back({device, device->id(), groupID, inputID});
      }

      if(auto item = inputList.selected()) {
        item.cell(1 + activeBinding).setIcon(Icon::Go::Right).setText({"(", keyboardChordText(pendingKeyboardChord), ")"});
      }

      armChordCommit();
      return;
    }

    if(oldValue != 0 && newValue == 0 && !pendingKeyboardChord.empty()) {
      armChordCommit();
    }
    return;
  }

  if(activeMapping->bind(activeBinding, device, groupID, inputID, oldValue, newValue)) {
    finishAssignment(*this);
  }
}

auto HotkeySettings::setVisible(bool visible) -> HotkeySettings& {
  if(visible == 1) refresh();
  if(visible == 0) {
    chordTimer.setEnabled(false);
    activeMapping.reset();
    pendingKeyboardChord.clear();
    assignLabel.setText();
    settingsWindow.setDismissable(true);
  }
  VerticalLayout::setVisible(visible);
  return *this;
}
