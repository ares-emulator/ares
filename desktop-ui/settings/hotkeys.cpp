static auto finishAssignment(HotkeySettings& self) -> void {
  self.chordTimer.setEnabled(false);
  self.activeMapping.reset();
  self.pendingChord.clear();
  self.assignLabel.setText();
  self.refresh();
  self.timer.onActivate([&] {
    self.timer.setEnabled(false);
    self.inputList.setFocused();
    settingsWindow.setDismissable(true);
  }).setInterval(200).setEnabled();
}

static auto chordKeyText(const HotkeySettings::ChordKey& key) -> string {
  auto device = key.device;
  if(!device) return {};
  if(key.groupID >= device->size()) return {};
  if(key.inputID >= device->group(key.groupID).size()) return {};

  auto name = device->group(key.groupID).input(key.inputID).name();

  if(device->isKeyboard()) {
    return name;
  }

  if(device->isJoypad()) {
    string output = device->group(key.groupID).name();
    output.append(" ", name);
    if(key.qualifier == InputMapping::Qualifier::Lo) output.append(".Lo");
    if(key.qualifier == InputMapping::Qualifier::Hi) output.append(".Hi");
    return output;
  }

  return name;
}

static auto chordText(const std::vector<HotkeySettings::ChordKey>& chord) -> string {
  string output;
  for(auto index : range(chord.size())) {
    auto name = chordKeyText(chord[index]);
    if(!name) continue;
    if(index) output.append("+");
    output.append(name);
  }
  return output;
}

static auto chordAssignment(const std::vector<HotkeySettings::ChordKey>& chord) -> string {
  if(chord.empty()) return {};

  string output;
  for(auto index : range(chord.size())) {
    auto& key = chord[index];
    if(index) output.append("+");
    output.append(string{"0x", hex(key.deviceID), "/", key.groupID, "/", key.inputID});
    if(key.qualifier == InputMapping::Qualifier::Lo) output.append("/Lo");
    if(key.qualifier == InputMapping::Qualifier::Hi) output.append("/Hi");
  }
  return output;
}

static auto chordContains(const std::vector<HotkeySettings::ChordKey>& chord, const HotkeySettings::ChordKey& candidate) -> bool {
  for(auto& key : chord) {
    if(key.deviceID == candidate.deviceID && key.groupID == candidate.groupID && key.inputID == candidate.inputID
    && key.qualifier == candidate.qualifier) return true;
  }
  return false;
}

static auto chordFromAssignment(string_view assignment) -> std::vector<HotkeySettings::ChordKey> {
  std::vector<HotkeySettings::ChordKey> chord;
  if(!assignment) return chord;

  auto parts = nall::split(assignment, "+");
  chord.reserve(parts.size());
  for(auto& part : parts) {
    auto binding = InputMapping::assignment(part.strip());
    if(!binding) {
      chord.clear();
      return chord;
    }
    chord.push_back(*binding);
  }

  return chord;
}

static auto chordKeyFromEvent(std::shared_ptr<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> maybe<HotkeySettings::ChordKey> {
  if(!device) return {};

  auto key = HotkeySettings::ChordKey{};
  key.device = device;
  key.deviceID = device->id();
  key.groupID = groupID;
  key.inputID = inputID;

  if(device->isKeyboard()) {
    if(groupID != HID::Keyboard::GroupID::Button) return {};
    if(oldValue == 0 && newValue != 0) return key;
    return {};
  }

  if(device->isJoypad()) {
    if(groupID == HID::Joypad::GroupID::Button) {
      if(oldValue == 0 && newValue != 0) return key;
      return {};
    }

    if(oldValue >= -16384 && newValue < -16384) {
      key.qualifier = InputMapping::Qualifier::Lo;
      return key;
    }

    if(oldValue <= +16384 && newValue > +16384) {
      key.qualifier = InputMapping::Qualifier::Hi;
      return key;
    }
  }

  return {};
}

static auto hotkeyAssignmentText(string_view assignment) -> string {
  if(!assignment) return {};

  auto chord = chordFromAssignment(assignment);
  if(chord.empty()) return {};

  string output;
  for(auto index : range(chord.size())) {
    auto& key = chord[index];
    if(!key.device) return "(disconnected)";
    if(key.groupID >= key.device->size()) return {};
    if(key.inputID >= key.device->group(key.groupID).size()) return {};
    if(index) output.append("+");
    output.append(chordKeyText(key));
  }

  return output;
}

static auto hotkeyAssignmentIcon(string_view assignment) -> multiFactorImage {
  if(!assignment) return {};

  bool hasKeyboard = false;
  bool hasMouse = false;
  bool hasJoypad = false;
  bool hasUnknown = false;

  auto chord = chordFromAssignment(assignment);
  if(chord.empty()) return {};

  for(auto& key : chord) {
    auto device = key.device;

    if(!device) {
      hasUnknown = true;
      continue;
    }

    if(device->isKeyboard()) hasKeyboard = true;
    else if(device->isMouse()) hasMouse = true;
    else if(device->isJoypad()) hasJoypad = true;
    else hasUnknown = true;
  }

  u32 kinds = hasKeyboard + hasMouse + hasJoypad + hasUnknown;
  if(kinds > 1) return Icon::Place::Settings;
  if(hasKeyboard) return Icon::Device::Keyboard;
  if(hasMouse) return Icon::Device::Mouse;
  if(hasJoypad) return Icon::Device::Joypad;
  return Icon::Place::Settings;
}

static constexpr u32 ChordKeyLimit = 3;

static auto isStateSlotHotkeyName(string_view name) -> bool {
  string suffix{name};
  if(suffix.beginsWith("Save State ")) suffix.trimLeft("Save State ", 1L);
  else if(suffix.beginsWith("Load State ")) suffix.trimLeft("Load State ", 1L);
  else return false;

  auto slot = suffix.natural();
  return slot >= 1 && slot <= 9;
}

auto HotkeySettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  stateSlotHotkeysToggle.setText("Show Save/Load Slot Hotkeys").onToggle([&] { eventToggleStateSlotHotkeys(); });
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
  visibleHotkeyIndices.clear();
  inputList.append(TableViewColumn().setText("Name"));
  for(u32 binding : range(BindingLimit)) {
    inputList.append(TableViewColumn().setText({"Mapping #", 1 + binding}).setExpandable());
  }

  for(u32 index : range(inputManager.hotkeys.size())) {
    auto& mapping = inputManager.hotkeys[index];
    if(!showStateSlotHotkeys && isStateSlotHotkeyName(mapping.name)) continue;
    visibleHotkeyIndices.push_back(index);
    TableViewItem item{&inputList};
    item.append(TableViewCell().setText(mapping.name).setFont(Font().setBold()));
    for(u32 binding : range(BindingLimit)) item.append(TableViewCell());
  }

  refresh();
  eventChange();
  inputList.resizeColumns();
}

auto HotkeySettings::refresh() -> void {
  for(u32 row : range(visibleHotkeyIndices.size())) {
    auto& mapping = inputManager.hotkeys[visibleHotkeyIndices[row]];
    for(u32 binding : range(BindingLimit)) {
      //do not remove identifier from mappings currently being assigned
      if(activeMapping && &activeMapping() == &mapping && activeBinding == binding) continue;
      auto cell = inputList.item(row).cell(1 + binding);
      auto assignment = mapping.assignments[binding];
      if(nall::split(assignment, "+").size() > 1) {
        cell.setIcon(Icon::Device::Keyboard);
        cell.setText(hotkeyAssignmentText(assignment));
      } else {
        cell.setIcon(mapping.bindings[binding].icon());
        cell.setText(mapping.bindings[binding].text());
      }
    }
  }
}

auto HotkeySettings::eventChange() -> void {
  Program::Guard guard;
  assignButton.setEnabled(inputList.batched().size() == 1);
  clearButton.setEnabled(inputList.batched().size() >= 1);
}

auto HotkeySettings::eventToggleStateSlotHotkeys() -> void {
  showStateSlotHotkeys = stateSlotHotkeysToggle.checked();

  if(activeMapping && !showStateSlotHotkeys && isStateSlotHotkeyName(activeMapping->name)) {
    chordTimer.setEnabled(false);
    activeMapping.reset();
    pendingChord.clear();
    assignLabel.setText();
    settingsWindow.setDismissable(true);
  }

  reload();
}

auto HotkeySettings::eventClear() -> void {
  for(auto& item : inputList.batched()) {
    auto& mapping = inputManager.hotkeys[visibleHotkeyIndices[item.offset()]];
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
    activeMapping = inputManager.hotkeys[visibleHotkeyIndices[item.offset()]];
    activeBinding = max(0, (s32)cell.offset() - 1);
    pendingChord.clear();

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
    if(!activeMapping || pendingChord.empty()) return;
    activeMapping->bind(activeBinding, chordAssignment(pendingChord));
    finishAssignment(*this);
  };
  auto armChordCommit = [&] {
    chordTimer.onActivate(commitPendingChord).setInterval(1200).setEnabled();
  };

  if(device->isKeyboard()) {
    if(groupID != HID::Keyboard::GroupID::Button) return;

    auto name = device->group(groupID).input(inputID).name();
    if(name == "Escape" && oldValue == 0 && newValue != 0) {
      activeMapping->unbind(activeBinding);
      finishAssignment(*this);
      return;
    }
  }

  if(auto key = chordKeyFromEvent(device, groupID, inputID, oldValue, newValue)) {
    if(!chordContains(pendingChord, *key) && pendingChord.size() < ChordKeyLimit) {
      pendingChord.push_back(*key);
    }

    if(auto item = inputList.selected()) {
      item.cell(1 + activeBinding).setIcon(Icon::Go::Right).setText({"(", chordText(pendingChord), ")"});
    }

    if(pendingChord.size() >= ChordKeyLimit) {
      commitPendingChord();
      return;
    }

    armChordCommit();
    return;
  }

  if(oldValue != 0 && newValue == 0 && !pendingChord.empty()) {
    armChordCommit();
  }
}

auto HotkeySettings::setVisible(bool visible) -> HotkeySettings& {
  if(visible == 1) refresh();
  if(visible == 0) {
    chordTimer.setEnabled(false);
    activeMapping.reset();
    pendingChord.clear();
    assignLabel.setText();
    settingsWindow.setDismissable(true);
  }
  VerticalLayout::setVisible(visible);
  return *this;
}
