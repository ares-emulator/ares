#include "../lucia.hpp"
#include "hotkeys.cpp"

VirtualPad virtualPads[2];
InputManager inputManager;

auto InputMapping::bind() -> void {
  for(auto& binding : bindings) binding = {};

  for(u32 index : range(BindingLimit)) {
    auto& assignment = assignments[index];
    auto& binding = bindings[index];

    auto token = assignment.split("/");
    if(token.size() < 3) continue;  //ignore invalid mappings

    binding.deviceID = token[0].natural();
    binding.groupID = token[1].natural();
    binding.inputID = token[2].natural();
    binding.qualifier = Qualifier::None;
    if(token(3) == "Lo") binding.qualifier = Qualifier::Lo;
    if(token(3) == "Hi") binding.qualifier = Qualifier::Hi;
    if(token(3) == "Rumble") binding.qualifier = Qualifier::Rumble;

    for(auto& device : inputManager.devices) {
      if(binding.deviceID == device->id()) {
        binding.device = device;
        break;
      }
    }
  }
}

auto InputMapping::bind(u32 binding, string assignment) -> void {
  if(binding >= BindingLimit) return;
  assignments[binding] = assignment;
  bind();
}

auto InputMapping::unbind() -> void {
  for(u32 binding : range(BindingLimit)) unbind(binding);
}

auto InputMapping::unbind(u32 binding) -> void {
  if(binding >= BindingLimit) return;
  bindings[binding] = {};
  assignments[binding] = {};
}

auto InputMapping::Binding::icon() -> image {
  if(!device && deviceID) return Icon::Device::Joypad;
  if(!device) return {};
  if(device->isKeyboard()) return Icon::Device::Keyboard;
  if(device->isMouse()) return Icon::Device::Mouse;
  if(device->isJoypad()) return Icon::Device::Joypad;
  return {};
}

auto InputMapping::Binding::text() -> string {
  if(!device && deviceID) return "(disconnected)";
  if(!device) return {};
  if(groupID >= device->size()) return {};
  if(inputID >= device->group(groupID).size()) return {};

  if(device->isKeyboard()) {
    return device->group(groupID).input(inputID).name();
  }

  if(device->isMouse()) {
    return device->group(groupID).input(inputID).name();
  }

  if(device->isJoypad()) {
    string name{Hash::CRC16(string{device->id()}).digest().upcase()};
    if(device->vendorID() == 0x045e && device->productID() == 0x028e) {
      name = {"Xbox360{", 1 + device->pathID(), "}"};
    } else {
      name.append(" ", device->group(groupID).name());
    }
    name.append(" ", device->group(groupID).input(inputID).name());
    if(qualifier == Qualifier::Lo) name.append(".Lo");
    if(qualifier == Qualifier::Hi) name.append(".Hi");
    if(qualifier == Qualifier::Rumble) name.append(".Rumble");
    return name;
  }

  return {};
}

//

auto InputButton::bind(u32 binding, shared_pointer<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
  string assignment = {"0x", hex(device->id()), "/", groupID, "/", inputID};

  if(device->isNull()) {
    return unbind(binding), true;
  }

  if(device->isKeyboard() && device->group(groupID).input(inputID).name() == "Escape") {
    return unbind(binding), true;
  }

  if(device->isKeyboard() && oldValue == 0 && newValue == 1) {
    return bind(binding, assignment), true;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Button && oldValue == 0 && newValue == 1) {
    return bind(binding, assignment), true;
  }

  if(device->isJoypad() && groupID != HID::Joypad::GroupID::Button && (
    (oldValue >= -16384 && newValue < -16384) || (oldValue <= -16384 && newValue > -16384)
  )) {
    return bind(binding, {assignment, "/Lo"}), true;
  }

  if(device->isJoypad() && groupID != HID::Joypad::GroupID::Button && (
    (oldValue <= +16384 && newValue > +16384) || (oldValue >= +16384 && newValue < +16384)
  )) {
    return bind(binding, {assignment, "/Hi"}), true;
  }

  return false;
}

auto InputButton::value() -> s16 {
  s16 result = 0;

  for(auto& binding : bindings) {
    if(!binding.device) continue;  //unbound

    auto& device = binding.device;
    auto& groupID = binding.groupID;
    auto& inputID = binding.inputID;
    auto& qualifier = binding.qualifier;
    s16 value = device->group(groupID).input(inputID).value();
    s16 output = 0;

    if(device->isKeyboard() && groupID == HID::Keyboard::GroupID::Button) {
      output = value != 0;
    }

    if(device->isJoypad() && groupID == HID::Joypad::GroupID::Button) {
      output = value != 0;
    }

    if(device->isJoypad() && groupID == HID::Joypad::GroupID::Hat) {
      if(qualifier == Qualifier::Lo) output = value < -16384;
      if(qualifier == Qualifier::Hi) output = value > +16384;
    }

    if(device->isJoypad() && groupID == HID::Joypad::GroupID::Axis) {
      if(qualifier == Qualifier::Lo) output = value < -16384;
      if(qualifier == Qualifier::Hi) output = value > +16384;
    }

    if(device->isJoypad() && groupID == HID::Joypad::GroupID::Trigger) {
      if(qualifier == Qualifier::Lo) output = value < -16384;
      if(qualifier == Qualifier::Hi) output = value > +16384;
    }

    result |= output;
  }

  return result;
}

//

auto InputAxis::bind(u32 binding, shared_pointer<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
  string assignment = {"0x", hex(device->id()), "/", groupID, "/", inputID};

  if(device->isNull()) {
    return unbind(binding), true;
  }

  if(device->isKeyboard() && device->group(groupID).input(inputID).name() == "Escape") {
    return unbind(binding), true;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Axis && (
    (oldValue >= -16384 && newValue < -16384) || (oldValue <= -16384 && newValue > -16384)
  )) {
    return bind(binding, assignment), true;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Axis && (
    (oldValue <= +16384 && newValue > +16384) || (oldValue >= +16384 && newValue < +16384)
  )) {
    return bind(binding, assignment), true;
  }

  return false;
}

auto InputAxis::value() -> s16 {
  s16 result = 0;

  for(auto& binding : bindings) {
    if(!binding.device) continue;  //unbound

    auto& device = binding.device;
    auto& groupID = binding.groupID;
    auto& inputID = binding.inputID;
    auto& qualifier = binding.qualifier;
    s16 value = device->group(groupID).input(inputID).value();

    if(device->isJoypad() && groupID == HID::Joypad::GroupID::Axis) {
      result += value;
    }
  }

  return result;
}

//

auto InputRumble::bind(u32 binding, shared_pointer<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
  string assignment = {"0x", hex(device->id()), "/", groupID, "/", inputID};

  if(device->isNull()) {
    return unbind(binding), true;
  }

  if(device->isKeyboard() && device->group(groupID).input(inputID).name() == "Escape") {
    return unbind(binding), true;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Button && oldValue == 0 && newValue == 1) {
    return bind(binding, assignment), true;
  }

  return false;
}

auto InputRumble::value() -> s16 {
  return 0;
}

auto InputRumble::rumble(bool enable) -> void {
  for(auto& binding : bindings) {
    if(!binding.device) continue;
    ruby::input.rumble(binding.deviceID, enable);
  }
}

//

VirtualPad::VirtualPad() {
  mappings.append(&up);
  mappings.append(&down);
  mappings.append(&left);
  mappings.append(&right);
  mappings.append(&select);
  mappings.append(&start);
  mappings.append(&a);
  mappings.append(&b);
  mappings.append(&c);
  mappings.append(&x);
  mappings.append(&y);
  mappings.append(&z);
  mappings.append(&l1);
  mappings.append(&r1);
  mappings.append(&l2);
  mappings.append(&r2);
  mappings.append(&lt);
  mappings.append(&rt);
  mappings.append(&lx);
  mappings.append(&ly);
  mappings.append(&rx);
  mappings.append(&ry);
  mappings.append(&rumble);
}

//

auto InputManager::create() -> void {
  createHotkeys();
}

auto InputManager::bind() -> void {
  for(auto& virtualPad : virtualPads) {
    for(auto& mapping : virtualPad.mappings) mapping->bind();
  }
  for(auto& mapping : hotkeys) mapping.bind();
}

auto InputManager::poll(bool force) -> void {
  //polling actual hardware is very time-consuming; skip call if poll was called too recently
  auto thisPoll = chrono::millisecond();
  if(thisPoll - lastPoll < pollFrequency && !force) return;
  lastPoll = thisPoll;

  auto devices = ruby::input.poll();
  bool changed = devices.size() != this->devices.size();
  if(!changed) {
    for(u32 index : range(devices.size())) {
      changed = devices[index] != this->devices[index];
      if(changed) break;
    }
  }
  if(changed) {
    this->devices = devices;
    bind();
    inputSettings.refresh();
    hotkeySettings.refresh();
  }
}

auto InputManager::eventInput(shared_pointer<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> void {
  inputSettings.eventInput(device, groupID, inputID, oldValue, newValue);
  hotkeySettings.eventInput(device, groupID, inputID, oldValue, newValue);
}
