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

static auto keyboardChordText(const std::vector<InputMapping::Binding::Chord>& chord) -> string {
  string output;
  for(auto index : range(chord.size())) {
    auto name = keyboardName(chord[index].device, chord[index].groupID, chord[index].inputID);
    if(!name) continue;
    if(index) output.append("+");
    output.append(name);
  }
  return output;
}

static auto keyboardChordAssignment(const std::vector<InputMapping::Binding::Chord>& chord) -> string {
  if(chord.empty()) return {};

  string output;
  for(auto index : range(chord.size())) {
    auto& key = chord[index];
    if(index) output.append("+");
    output.append(string{"0x", hex(key.deviceID), "/", key.groupID, "/", key.inputID});
  }
  return output;
}

static auto keyboardChordContains(const std::vector<InputMapping::Binding::Chord>& chord, u32 groupID, u32 inputID) -> bool {
  for(auto& key : chord) {
    if(key.groupID == groupID && key.inputID == inputID) return true;
  }
  return false;
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
      cell.setIcon(mapping.bindings[binding].icon());
      cell.setText(mapping.bindings[binding].text());
    }
    index++;
  }
}

auto HotkeySettings::eventChange() -> void {
  Program::Guard guard;
  assignButton.setEnabled(inputList.batched().size() == 1);
  clearButton.setEnabled(inputList.batched().size() >= 1);
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
