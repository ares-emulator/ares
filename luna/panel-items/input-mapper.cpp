InputMapper::InputMapper(View* view) : PanelItem(view, Size{~0, ~0}) {
  setCollapsible().setVisible(false);
  nameLabel.setFont(Font().setBold());
  inputList.setBatchable();
  inputList.onActivate([&](auto cell) { eventAssign(); });
  inputList.onChange([&] { eventChange(); });
  mouseXaxis.setText("Mouse X-axis").setCollapsible().onActivate([&] { eventAssignMouse(0, 0); });
  mouseYaxis.setText("Mouse Y-axis").setCollapsible().onActivate([&] { eventAssignMouse(0, 1); });
  mouseLeft.setText("Mouse Left").setCollapsible().onActivate([&] { eventAssignMouse(1, 0); });
  mouseMiddle.setText("Mouse Middle").setCollapsible().onActivate([&] { eventAssignMouse(1, 1); });
  mouseRight.setText("Mouse Right").setCollapsible().onActivate([&] { eventAssignMouse(1, 2); });
  assignButton.setText("Assign").onActivate([&] { eventAssign(); });
  clearButton.setText("Clear").onActivate([&] { eventClear(); });
}

auto InputMapper::show() -> void {
  setVisible(true);
}

auto InputMapper::hide() -> void {
  setVisible(false);
  node = {};
  inputList.reset();
}

auto InputMapper::refresh(ares::Node::Object node) -> void {
  this->node = node;

  nameLabel.setText(node->name());
  inputList.reset().setEnabled();
  inputList.append(TableViewColumn().setText("Name"));
  inputList.append(TableViewColumn().setText("Mapping").setExpandable());
  for(auto& node : node->find<ares::Node::Input>()) {
    if(node->parent() != this->node) continue;
    TableViewItem item{&inputList};
    item.setAttribute<ares::Node::Input>("node", node);
    TableViewCell name{&item};
    name.setText(node->name()).setFont(Font().setBold());
    TableViewCell value{&item};
  }
  update();
  inputList.doChange();
}

auto InputMapper::update() -> void {
  for(auto& item : inputList.items()) {
    auto value = item.cell(1);
    auto node = item.attribute<ares::Node::Input>("node");
    if(auto name = node->attribute("inputName")) {
      auto device = node->attribute("deviceName");
      if(device == "Keyboard") value.setIcon(Icon::Device::Keyboard);
      else if(device == "Mouse") value.setIcon(Icon::Device::Mouse);
      else if(device == "Joypad") value.setIcon(Icon::Device::Joypad);
      else value.setIcon(Icon::Action::Close);
      string label;
      if(label == "Joypad") label.append(node->attribute("groupName"), ".");
      label.append(node->attribute("inputName"));
      if(auto qualifier = node->attribute("qualifier")) label.append(".", qualifier);
      value.setText(label);
    } else {
      value.setIcon(Icon::Action::Close);
      value.setText("(unmapped)");
    }
  }

  inputList.resizeColumns();
}

auto InputMapper::eventAssignMouse(uint groupID, uint inputID) -> void {
  auto batched = inputList.batched();
  if(batched.size() != 1) return;
  for(auto& device : inputManager.devices) {
    if(!device->isMouse()) continue;
    assigning = batched.first().attribute<ares::Node::Input>("node");
    eventInput(device, groupID, inputID, 0, 1, true);
    return;
  }
}

auto InputMapper::eventAssign() -> void {
  //prevent an activation via the return key from mapping return instantly
  inputManager.poll();

  assigningQueue = inputList.batched();  //assign selected
  if(!assigningQueue) assigningQueue = inputList.items();  //assign all
  if(!assigningQueue) return;  //no inputs to assign

  eventAssignNext();
}

auto InputMapper::eventAssignNext() -> void {
  if(!assigningQueue) {
    inputList.selectNone().setFocused();
    return eventChange();
  }

  //use the viewport to sink inputs away from the table view during assignment
  program.viewport.setFocused();
  auto item = assigningQueue.takeFirst();
  inputList.selectNone();
  item.setSelected().setFocused();  //scroll the current assigning mapping into view
  auto input = item.attribute<ares::Node::Input>("node");
  item.cell(1).setIcon(Icon::Go::Right).setText("(assign)");
  assigning = input;
  eventChange();
}

auto InputMapper::eventClear() -> void {
  auto batched = inputList.batched();  //clear selected
  if(!batched) batched = inputList.items();  //clear all
  if(!batched) return;  //no inputs to clear

  for(auto& item : batched) {
    auto input = item.attribute<ares::Node::Input>("node");
    input->setAttribute("pathID");
    input->setAttribute("vendorID");
    input->setAttribute("productID");
    input->setAttribute("groupID");
    input->setAttribute("inputID");
    input->setAttribute("deviceName");
    input->setAttribute("groupName");
    input->setAttribute("inputName");
    input->setAttribute("qualifier");
  }
  inputManager.bind();
  update();
}

auto InputMapper::eventChange() -> void {
  auto batched = inputList.batched();
  bool showMouseAxes = false;
  bool showMouseButtons = false;
  if(batched.size() == 1 && assigning) {
    if(auto node = batched.first().attribute<ares::Node::Input>("node")) {
      if(node->cast<ares::Node::Axis>()) showMouseAxes = true;
      if(node->cast<ares::Node::Button>()) showMouseButtons = true;
    }
  }
  mouseXaxis.setVisible(showMouseAxes);
  mouseYaxis.setVisible(showMouseAxes);
  mouseLeft.setVisible(showMouseButtons);
  mouseMiddle.setVisible(showMouseButtons);
  mouseRight.setVisible(showMouseButtons);
  controlLayout.resize();
  assignButton.setText((bool)batched ? "Assign" : "Assign All");
  clearButton.setText((bool)batched ? "Clear" : "Clear All");
}

auto InputMapper::eventInput(shared_pointer<HID::Device> device, uint group, uint input, int16_t oldValue, int16_t newValue, bool allowMouseInput) -> void {
  if(!assigning) return;

  auto isButton  = (bool)assigning->cast<ares::Node::Button>();
  auto isAxis    = (bool)assigning->cast<ares::Node::Axis>();
  auto isTrigger = (bool)assigning->cast<ares::Node::Trigger>();
  auto isRumble  = (bool)assigning->cast<ares::Node::Rumble>();

  bool allow = false;
  string qualifier;

  if(isButton) {
    if(device->isKeyboard()) {
      if(oldValue == 0 && newValue == 1) allow = true;
    }
    if(device->isJoypad() && (group == HID::Joypad::Axis || group == HID::Joypad::Hat)) {
      if(newValue < -24576) allow = true, qualifier = "Lo";
      if(newValue > +24576) allow = true, qualifier = "Hi";
    }
    if(device->isJoypad() && group == HID::Joypad::Button) {
      if(oldValue == 0 && newValue == 1) allow = true;
    }
  }

  if(isAxis) {
  }

  if(isTrigger) {
  }

  if(isRumble) {
  }

  if(allowMouseInput && device->isMouse()) allow = true;
  if(!allow) return;

  if(device->group(group).input(input).name() == "Escape") {
    assigning->setAttribute("pathID");
    assigning->setAttribute("vendorID");
    assigning->setAttribute("productID");
    assigning->setAttribute("groupID");
    assigning->setAttribute("inputID");
    assigning->setAttribute("deviceName");
    assigning->setAttribute("groupName");
    assigning->setAttribute("inputName");
    assigning->setAttribute("qualifier");
  } else {
    assigning->setAttribute("pathID", device->pathID());
    assigning->setAttribute("vendorID", device->vendorID());
    assigning->setAttribute("productID", device->productID());
    assigning->setAttribute("groupID", group);
    assigning->setAttribute("inputID", input);
    assigning->setAttribute("deviceName", device->name());
    assigning->setAttribute("groupName", device->group(group).name());
    assigning->setAttribute("inputName", device->group(group).input(input).name());
    assigning->setAttribute("qualifier", qualifier);
  }
  assigning.reset();
  inputManager.bind();
  update();
  eventAssignNext();
}
