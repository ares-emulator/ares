auto InputSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  inputLabel.setText("Virtual Gamepad Bindings").setFont(Font().setBold());
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

auto InputSettings::reload() -> void {
  inputList.reset();
  inputList.append(TableViewColumn().setText("Name"));
  for(uint binding : range(BindingLimit)) {
    inputList.append(TableViewColumn().setText({"Mapping #", 1 + binding}).setExpandable());
  }

  for(auto& mapping : virtualPad.mappings) {
    TableViewItem item{&inputList};
    item.append(TableViewCell().setText(mapping->name).setFont(Font().setBold()));
    for(uint binding : range(BindingLimit)) item.append(TableViewCell());
  }

  refresh();
  eventChange();
}

auto InputSettings::refresh() -> void {
  uint index = 0;
  for(auto& mapping : virtualPad.mappings) {
    for(uint binding : range(BindingLimit)) {
      //do not remove identifier from mappings currently being assigned
      if(activeMapping && &activeMapping() == mapping && activeBinding == binding) continue;
      auto cell = inputList.item(index).cell(1 + binding);
      cell.setIcon(mapping->bindings[binding].icon());
      cell.setText(mapping->bindings[binding].text());
    }
    index++;
  }
}

auto InputSettings::eventChange() -> void {
  assignButton.setEnabled(inputList.batched().size() == 1);
  clearButton.setEnabled(inputList.batched().size() >= 1);
}

auto InputSettings::eventClear() -> void {
  for(auto& item : inputList.batched()) {
    auto& mapping = *virtualPad.mappings[item.offset()];
    mapping.unbind();
  }
  refresh();
}

auto InputSettings::eventAssign(TableViewCell cell) -> void {
  inputManager.poll(true);  //clear any pending events first

  if(ruby::input.driver() == "None") return (void)MessageDialog().setText(
    "Bindings cannot be set when no input driver has been loaded.\n"
    "Please go to driver settings and activate an input driver first."
  ).setAlignment(settingsWindow).error();

  if(auto item = inputList.selected()) {
    if(activeMapping) refresh();  //clear any previous assign arrow prompts
    activeMapping = *virtualPad.mappings[item.offset()];
    activeBinding = max(0, (int)cell.offset() - 1);

    item.cell(1 + activeBinding).setIcon(Icon::Go::Right).setText("(assign ...)");
    assignLabel.setText({"Press a key or button for mapping #", 1 + activeBinding, " [", activeMapping->name, "] ..."});
    refresh();
    settingsWindow.setDismissable(false);
    Application::processEvents();
    spacer.setFocused();
  }
}

auto InputSettings::eventInput(shared_pointer<HID::Device> device, uint groupID, uint inputID, int16_t oldValue, int16_t newValue) -> void {
  if(!activeMapping) return;
  if(!settingsWindow.focused()) return;

  if(activeMapping->bind(activeBinding, device, groupID, inputID, oldValue, newValue)) {
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
  if(visible == 1) refresh();
  if(visible == 0) activeMapping.reset(), assignLabel.setText(), settingsWindow.setDismissable(true);
  VerticalLayout::setVisible(visible);
  return *this;
}
