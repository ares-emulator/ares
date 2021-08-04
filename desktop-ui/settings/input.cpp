auto InputSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  systemList.append(ComboButtonItem().setText("Virtual Gamepads"));
  for(auto& emulator : emulators) {
    systemList.append(ComboButtonItem().setText(emulator->name));
  }
  systemList.onChange([&] { systemChange(); });
  portList.onChange([&] { portChange(); });
  deviceList.onChange([&] { deviceChange(); });
  inputList.setBatchable();
  inputList.setHeadered();
  inputList.onContext([&](auto cell) { eventContext(cell); });
  inputList.onChange([&] { eventChange(); });
  inputList.onActivate([&](auto cell) { eventAssign(cell); });

  systemChange();

  assignLabel.setFont(Font().setBold());
  spacer.setFocusable();
  assignButton.setText("Assign").onActivate([&] { eventAssign(inputList.selected().cell(0)); });
  clearButton.setText("Clear").onActivate([&] { eventClear(); });
}

auto InputSettings::systemChange() -> void {
  portList.reset();
  auto& ports = Emulator::enumeratePorts(systemList.selected().text());
  for(auto& port : ports) {
    portList.append(ComboButtonItem().setText(port.name));
  }
  portChange();
}

auto InputSettings::portChange() -> void {
  deviceList.reset();
  auto& ports = Emulator::enumeratePorts(systemList.selected().text());
  auto& port = ports[portList.selected().offset()];
  for(auto& device : port.devices) {
    deviceList.append(ComboButtonItem().setText(device.name));
  }
  deviceChange();
}

auto InputSettings::deviceChange() -> void {
  inputList.reset();
  inputList.append(TableViewColumn().setText("Name"));
  for(u32 binding : range(BindingLimit)) {
    inputList.append(TableViewColumn().setText({"Mapping #", 1 + binding}).setExpandable());
  }

  auto& ports = Emulator::enumeratePorts(systemList.selected().text());
  auto& port = ports[portList.selected().offset()];
  auto& device = port.devices[deviceList.selected().offset()];
  for(auto& input : device.inputs) {
    TableViewItem item{&inputList};
    item.setAttribute<u32>("type", (u32)input.type);
    item.append(TableViewCell().setText(input.name).setFont(Font().setBold()));
    for(u32 binding : range(BindingLimit)) item.append(TableViewCell());
  }

  refresh();
  eventChange();
  Application::processEvents();
  inputList.resizeColumns();
}

auto InputSettings::refresh() -> void {
  u32 index = 0;
  auto& ports = Emulator::enumeratePorts(systemList.selected().text());
  auto& port = ports[portList.selected().offset()];
  auto& device = port.devices[deviceList.selected().offset()];
  for(auto& input : device.inputs) {
    for(u32 binding : range(BindingLimit)) {
      //do not remove identifier from mappings currently being assigned
      if(activeMapping && &activeMapping() == &input && activeBinding == binding) continue;
      auto cell = inputList.item(index).cell(1 + binding);
      cell.setIcon(input.mapping->bindings[binding].icon());
      cell.setText(input.mapping->bindings[binding].text());
    }
    index++;
  }
}

auto InputSettings::eventContext(TableViewCell cell) -> void {
  if(!cell) return;
  if(cell.offset() == 0) return;  //ignore label cell
  if(cell.offset() >= BindingLimit) return;  //should never occur

  auto item = cell->parentTableViewItem();
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
  assignButton.setEnabled(inputList.batched().size() == 1);
  clearButton.setEnabled(inputList.batched().size() >= 1);
}

auto InputSettings::eventClear() -> void {
  auto& ports = Emulator::enumeratePorts(systemList.selected().text());
  auto& port = ports[portList.selected().offset()];
  auto& device = port.devices[deviceList.selected().offset()];
  for(auto& item : inputList.batched()) {
    auto& mapping = *device.inputs[item.offset()].mapping;
    mapping.unbind();
  }
  refresh();
}

auto InputSettings::eventAssign(TableViewCell cell, string binding) -> void {
  if(ruby::input.driver() == "None") return (void)MessageDialog().setText(
    "Bindings cannot be set when no input driver has been loaded.\n"
    "Please go to driver settings and activate an input driver first."
  ).setAlignment(settingsWindow).error();

  auto& ports = Emulator::enumeratePorts(systemList.selected().text());
  auto& port = ports[portList.selected().offset()];
  auto& device = port.devices[deviceList.selected().offset()];
  if(auto item = inputList.selected()) {
    if(activeMapping) refresh();  //clear any previous assign arrow prompts
    activeMapping = device.inputs[item.offset()];
    activeBinding = max(0, (s32)cell.offset() - 1);

    for(auto device : inputManager.devices) {
      if(device->isMouse()) {
        for(auto& group : *device) {
          if(auto inputID = group.find(binding)) {
            auto groupID = (binding == "X" || binding == "Y") ? HID::Mouse::GroupID::Axis : HID::Mouse::GroupID::Button;
            activeMapping->mapping->bind(activeBinding, device, groupID, inputID(), 0, 1);
          }
        }
      }
    }

    activeMapping.reset();
    refresh();
  }
}

auto InputSettings::eventAssign(TableViewCell cell) -> void {
  inputManager.poll(true);  //clear any pending events first

  if(ruby::input.driver() == "None") return (void)MessageDialog().setText(
    "Bindings cannot be set when no input driver has been loaded.\n"
    "Please go to driver settings and activate an input driver first."
  ).setAlignment(settingsWindow).error();

  auto& ports = Emulator::enumeratePorts(systemList.selected().text());
  auto& port = ports[portList.selected().offset()];
  auto& device = port.devices[deviceList.selected().offset()];
  if(auto item = inputList.selected()) {
    if(activeMapping) refresh();  //clear any previous assign arrow prompts
    activeMapping = device.inputs[item.offset()];
    activeBinding = max(0, (s32)cell.offset() - 1);

    item.cell(1 + activeBinding).setIcon(Icon::Go::Right).setText("(assign ...)");
    assignLabel.setText({"Press a key or button for mapping #", 1 + activeBinding, " [", activeMapping->name, "] ..."});
    refresh();
    settingsWindow.setDismissable(false);
    Application::processEvents();
    spacer.setFocused();
  }
}

auto InputSettings::eventInput(shared_pointer<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> void {
  if(!activeMapping) return;
  if(!settingsWindow.focused()) return;
  if(device->isMouse()) return;

  if(activeMapping->mapping->bind(activeBinding, device, groupID, inputID, oldValue, newValue)) {
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
  if(visible == 1) {
    systemList.items().first().setSelected();
    if(emulator) {
      for(auto item : systemList.items()) {
        if(item.text() == emulator->name) item.setSelected();
      }
    }
    systemList.doChange();
  }
  if(visible == 0) activeMapping.reset(), assignLabel.setText(), settingsWindow.setDismissable(true);
  VerticalLayout::setVisible(visible);
  return *this;
}
