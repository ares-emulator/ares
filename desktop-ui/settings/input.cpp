auto InputSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  inputDefocusLabel.setText("When focus is lost:").setFont(Font().setBold());
  inputDefocusPause.setText("Pause emulation").onActivate([&] {
    settings.input.defocus = "Pause";
  });
  inputDefocusBlock.setText("Block input").onActivate([&] {
    settings.input.defocus = "Block";
  });
  inputDefocusAllow.setText("Allow input").onActivate([&] {
    settings.input.defocus = "Allow";
  });
  if(settings.input.defocus == "Pause") inputDefocusPause.setChecked();
  if(settings.input.defocus == "Block") inputDefocusBlock.setChecked();
  if(settings.input.defocus == "Allow") inputDefocusAllow.setChecked();

  digitalToAnalogLabel.setText("Digital to analog:").setFont(Font().setBold());
  digitalToAnalogImmediate.setText("Immediate").onActivate([&] {
    settings.input.digitalToAnalog = "Immediate";
    refreshDigitalToAnalog();
  });
  digitalToAnalogGradualReturn.setText("Gradual (return to center)").onActivate([&] {
    settings.input.digitalToAnalog = "GradualReturn";
    refreshDigitalToAnalog();
  });
  digitalToAnalogGradualHold.setText("Gradual (hold position)").onActivate([&] {
    settings.input.digitalToAnalog = "GradualHold";
    refreshDigitalToAnalog();
  });
  if(settings.input.digitalToAnalog == "GradualReturn") {
    digitalToAnalogGradualReturn.setChecked();
  } else if(settings.input.digitalToAnalog == "GradualHold") {
    digitalToAnalogGradualHold.setChecked();
  } else {
    digitalToAnalogImmediate.setChecked();
  }

  digitalToAnalogTimeLabel.setText("Center-to-edge time:").setFont(Font().setBold());
  digitalToAnalogTimeSlider.setLength(19).setPosition((settings.input.digitalToAnalogTime - 100) / 50).onChange([&] {
    settings.input.digitalToAnalogTime = 100 + digitalToAnalogTimeSlider.position() * 50;
    refreshDigitalToAnalog();
  });
  refreshDigitalToAnalog();

  refreshTargets();
  systemList.onChange([&] { if(!updatingTargets) systemChange(); });
  portList.onChange([&] { portChange(); });
  deviceList.onChange([&] { deviceChange(); });
  inputHint.setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
  inputList.setBatchable();
  inputList.setHeadered();
  inputList.onContext([&](auto cell) { eventContext(cell); });
  inputList.onChange([&] { eventChange(); });
  inputList.onActivate([&](auto cell) { eventAssign(cell); });

  systemChange();

  assignLabel.setFont(Font().setBold());
  spacer.setFocusable();
  assignButton.setText("Assign").onActivate([&] { eventAssign(inputList.selected().cell(0)); });
  resetGameButton.setText("Clear all").onActivate([&] { resetGameMappings(); });
  clearButton.setText("Clear").onActivate([&] { eventClear(); });
}

auto InputSettings::refreshDigitalToAnalog() -> void {
  auto gradual = settings.input.digitalToAnalog != "Immediate";
  digitalToAnalogTimeLabel.setEnabled(gradual);
  digitalToAnalogTimeSlider.setEnabled(gradual);
  digitalToAnalogTimeValue.setEnabled(gradual).setText({settings.input.digitalToAnalogTime, " ms"});
}

auto InputSettings::editingGame() -> bool {
  return systemList.selected() && systemList.selected().attribute<bool>("game");
}

auto InputSettings::selectedPorts() -> std::vector<InputPort>& {
  if(editingGame() && emulator && !emulator->gameInputPorts.empty()) return emulator->gameInputPorts;
  return Emulator::enumeratePorts(systemList.selected().attribute<string>("system"));
}

auto InputSettings::cancelAssignment() -> void {
  activeMapping.reset();
  timer.setEnabled(false);
  assignLabel.setText();
  settingsWindow.setDismissable(true);
}

auto InputSettings::refreshTargets(bool selectCurrentGame) -> void {
  cancelAssignment();
  auto previous = systemList.selected();
  string system = previous ? previous.attribute<string>("system") : string{};
  bool game = previous && previous.attribute<bool>("game");
  updatingTargets = true;
  systemList.reset();
  systemList.append(ComboButtonItem().setText("Virtual Gamepads").setAttribute<string>("system", ""));
  if(emulator && !emulator->gameInputPorts.empty()) {
    systemList.append(ComboButtonItem().setText({emulator->gameInputTitle, " (This game)"})
      .setAttribute<string>("system", emulator->name).setAttribute<bool>("game", true));
  }
  for(auto& entry : emulators) {
    systemList.append(ComboButtonItem().setText(entry->name).setAttribute<string>("system", entry->name));
  }
  if(selectCurrentGame && emulator) {
    system = emulator->name;
    game = !emulator->gameInputPorts.empty();
  }
  for(auto item : systemList.items()) {
    if(item.attribute<string>("system") == system && item.attribute<bool>("game") == game) item.setSelected();
  }
  updatingTargets = false;
}

auto InputSettings::resetGameMappings() -> void {
  if(!editingGame() || !emulator) return;
  Program::Guard guard;
  cancelAssignment();
  for(auto& port : emulator->gameInputPorts) {
    for(auto& device : port.devices) {
      for(auto& input : device.inputs) input.configuredMapping().unbind();
      for(auto& pair : device.pairs) {
        pair.configuredMappingLo().unbind();
        pair.configuredMappingHi().unbind();
      }
    }
  }
  emulator->processGameInputMappings(false);
  refresh();
}

auto InputSettings::systemChange() -> void {
  cancelAssignment();
  auto selectedPort = portList.selected().text();
  portList.reset();
  auto& ports = selectedPorts();
  for(auto& port : ports) {
    portList.append(ComboButtonItem().setText(port.name));
  }
  for(auto item : portList.items()) {
    if(item.text() == selectedPort) item.setSelected();
  }
  portChange();
}

auto InputSettings::portChange() -> void {
  cancelAssignment();
  auto selectedDevice = deviceList.selected().text();
  deviceList.reset();
  auto& ports = selectedPorts();
  auto& port = ports[portList.selected().offset()];
  for(auto& device : port.devices) {
    deviceList.append(ComboButtonItem().setText(device.name));
  }
  for(auto item : deviceList.items()) {
    if(item.text() == selectedDevice) item.setSelected();
  }
  deviceChange();
}

auto InputSettings::deviceChange() -> void {
  cancelAssignment();
  inputList.reset();
  inputList.append(TableViewColumn().setText("Name"));
  for(u32 binding : range(BindingLimit)) {
    inputList.append(TableViewColumn().setText({"Mapping #", 1 + binding}).setExpandable());
  }

  auto& ports = selectedPorts();
  auto& port = ports[portList.selected().offset()];
  auto& device = port.devices[deviceList.selected().offset()];
  if(systemList.selected().offset() == 0) {
    inputHint.setText("Map Virtual Gamepads to match your physical controller; compatible systems use these mappings automatically.");
  } else if(editingGame()) {
    inputHint.setText("Mappings apply only to this game. Gray mappings are inherited from system defaults.");
  } else if(device.mappingMode == MappingMode::Direct) {
    inputHint.setText("This device has unique inputs and does not inherit Virtual Gamepad mappings.");
  } else {
    if(systemList.selected().attribute<string>("system") == "Arcade") {
      inputHint.setText("Arcade mappings override Virtual Gamepad defaults; gray mappings are inherited from the Virtual Gamepad.");
    } else {
      inputHint.setText("System mappings override Virtual Gamepad defaults; gray mappings are inherited from the Virtual Gamepad.");
    }
  }
  clearButton.setText("Clear");
  resetGameButton.setVisible(editingGame()).setCollapsible(!editingGame());
  bool arcade = false;
  for(auto& emulator : emulators) {
    if(emulator->name == systemList.selected().attribute<string>("system")) arcade = emulator->arcade();
  }

  auto appendInput = [&](u32 index) {
    auto& input = device.inputs[index];
    TableViewItem item{&inputList};
    item.setAttribute<u32>("input", index);
    item.setAttribute<u32>("type", (u32)input.type);
    item.append(TableViewCell().setText(input.label ? input.label : input.name).setFont(Font().setBold()));
    for(u32 binding : range(BindingLimit)) item.append(TableViewCell());
  };

  if(arcade) {
    bool hasDigital = false, hasAnalog = false;
    for(auto& input : device.inputs) {
      if(input.type == InputNode::Type::Digital) hasDigital = true;
      else hasAnalog = true;
    }
    if(hasDigital) {
      for(u32 index : range(device.inputs.size())) {
        if(device.inputs[index].type == InputNode::Type::Digital) appendInput(index);
      }
    }
    if(hasAnalog) {
      for(u32 index : range(device.inputs.size())) {
        if(device.inputs[index].type != InputNode::Type::Digital) appendInput(index);
      }
    }
  } else {
    for(u32 index : range(device.inputs.size())) appendInput(index);
  }

  refresh();
  eventChange();
  Application::processEvents();
  inputList.resizeColumns();
}

auto InputSettings::refresh() -> void {
  auto& ports = selectedPorts();
  auto& port = ports[portList.selected().offset()];
  auto& device = port.devices[deviceList.selected().offset()];
  for(auto& item : inputList.items()) {
    auto index = item->attribute<u32>("input");
    if(index >= device.inputs.size()) continue;
    auto& input = device.inputs[index];
    for(u32 binding : range(BindingLimit)) {
      //do not remove identifier from mappings currently being assigned
      if(activeMapping && &activeMapping() == &input && activeBinding == binding) continue;
      auto cell = item.cell(1 + binding);
      auto& mapping = input.effectiveMapping();
      cell.setIcon(mapping.bindings[binding].icon());
      cell.setText(mapping.bindings[binding].text());
      if(input.inheritedMapping()) cell.setForegroundColor(SystemColor::PlaceholderText);
      else cell.setForegroundColor();
    }
  }
  bool overridden = false;
  if(editingGame() && emulator) {
    for(auto& port : emulator->gameInputPorts) {
      for(auto& device : port.devices) {
        for(auto& input : device.inputs) if(input.configuredMapping().assigned()) overridden = true;
        for(auto& pair : device.pairs) if(pair.configuredMappingLo().assigned() || pair.configuredMappingHi().assigned()) overridden = true;
      }
    }
  }
  resetGameButton.setEnabled(overridden);
}

auto InputSettings::eventContext(TableViewCell cell) -> void {
  if(!cell) return;
  if(cell.offset() == 0) return;  //ignore label cell
  if(cell.offset() > BindingLimit) return;  //should never occur

  auto item = cell->parentTableViewItem();
  if(item->attribute<u32>("input") == ~0u) return;
  auto type = (InputNode::Type)item->attribute<u32>("type");
  menu.setVisible(false);
  menu.reset();
  if(type == InputNode::Type::Relative) {
    menu.append(MenuItem().setIcon(Icon::Device::Mouse).setText("Mouse X-axis").onActivate([=] {
      settingsWindow.inputSettings.eventAssign(cell, "X");
    }));
    menu.append(MenuItem().setIcon(Icon::Device::Mouse).setText("Mouse Y-axis").onActivate([=] {
      settingsWindow.inputSettings.eventAssign(cell, "Y");
    }));
  }
  if(type == InputNode::Type::Digital) {
    menu.append(MenuItem().setIcon(Icon::Device::Mouse).setText("Mouse Left Button").onActivate([=] {
      settingsWindow.inputSettings.eventAssign(cell, "Left");
    }));
    menu.append(MenuItem().setIcon(Icon::Device::Mouse).setText("Mouse Middle Button").onActivate([=] {
      settingsWindow.inputSettings.eventAssign(cell, "Middle");
    }));
    menu.append(MenuItem().setIcon(Icon::Device::Mouse).setText("Mouse Right Button").onActivate([=] {
      settingsWindow.inputSettings.eventAssign(cell, "Right");
    }));
  }
  if(menu.actionCount()) menu.setVisible(true);
}

auto InputSettings::eventChange() -> void {
  u32 count = 0;
  for(auto& item : inputList.batched()) {
    if(item->attribute<u32>("input") != ~0u) count++;
  }
  assignButton.setEnabled(inputList.batched().size() == 1 && count == 1);
  clearButton.setEnabled(count >= 1);
}

auto InputSettings::eventClear() -> void {
  auto& ports = selectedPorts();
  auto& port = ports[portList.selected().offset()];
  auto& device = port.devices[deviceList.selected().offset()];
  for(auto& item : inputList.batched()) {
    auto index = item->attribute<u32>("input");
    if(index >= device.inputs.size()) continue;
    auto& mapping = device.inputs[index].configuredMapping();
    mapping.unbind();
  }
  refresh();
}

auto InputSettings::eventAssign(TableViewCell cell, string binding) -> void {
  if(ruby::input.driver() == "None") return (void)MessageDialog().setText(
    "Bindings cannot be set when no input driver has been loaded.\n"
    "Please go to driver settings and activate an input driver first."
  ).setAlignment(settingsWindow).error();

  auto& ports = selectedPorts();
  auto& port = ports[portList.selected().offset()];
  auto& device = port.devices[deviceList.selected().offset()];
  if(auto item = inputList.selected()) {
    auto index = item->attribute<u32>("input");
    if(index >= device.inputs.size()) return;
    if(activeMapping) refresh();  //clear any previous assign arrow prompts
    activeMapping = device.inputs[index];
    activeBinding = max(0, (s32)cell.offset() - 1);

    for(auto device : inputManager.devices) {
      if(device->isMouse()) {
        for(auto& group : *device) {
          if(auto inputID = group.find(binding)) {
            auto groupID = (binding == "X" || binding == "Y") ? HID::Mouse::GroupID::Axis : HID::Mouse::GroupID::Button;
            activeMapping->configuredMapping().bind(activeBinding, device, groupID, inputID(), 0, 1);
          }
        }
      }
    }

    activeMapping.reset();
    refresh();
  }
}

auto InputSettings::eventAssign(TableViewCell cell) -> void {
  Program::Guard guard;
  inputManager.poll(true);  //clear any pending events first

  if(ruby::input.driver() == "None") return (void)MessageDialog().setText(
    "Bindings cannot be set when no input driver has been loaded.\n"
    "Please go to driver settings and activate an input driver first."
  ).setAlignment(settingsWindow).error();

  auto& ports = selectedPorts();
  auto& port = ports[portList.selected().offset()];
  auto& device = port.devices[deviceList.selected().offset()];
  if(auto item = inputList.selected()) {
    auto index = item->attribute<u32>("input");
    if(index >= device.inputs.size()) return;
    if(activeMapping) refresh();  //clear any previous assign arrow prompts
    activeMapping = device.inputs[index];
    activeBinding = max(0, (s32)cell.offset() - 1);

    item.cell(1 + activeBinding).setIcon(Icon::Go::Right).setText("(assign ...)");
    assignLabel.setText({"Press a key or button for mapping #", 1 + activeBinding, " [", activeMapping->label ? activeMapping->label : activeMapping->name, "] ..."});
    refresh();
    settingsWindow.setDismissable(false);
    Application::processEvents();
    spacer.setFocused();
  }
}

auto InputSettings::eventInput(std::shared_ptr<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> void {
  if(!activeMapping) return;
  if(!settingsWindow.focused()) return;
  if(device->isMouse()) return;

  if(activeMapping->configuredMapping().bind(activeBinding, device, groupID, inputID, oldValue, newValue)) {
    activeMapping.reset();
    assignLabel.setText();
    refresh();
    timer.onActivate([&] {
      timer.setEnabled(false);
      inputList.setFocused();
      settingsWindow.setDismissable(true);
    }).setInterval(200).setEnabled();
  }
}

auto InputSettings::setVisible(bool visible) -> InputSettings& {
  if(visible) refreshTargets(true), systemChange();
  else cancelAssignment();
  VerticalLayout::setVisible(visible);
  return *this;
}

auto InputSettings::inputDriverUpdate() -> bool {
  Program::Guard guard;
  program.inputDriverUpdate();
  return true;
}
