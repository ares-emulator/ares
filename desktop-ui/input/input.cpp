#include "../desktop-ui.hpp"
#include "hotkeys.cpp"

VirtualPort virtualPorts[2];
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

auto InputDigital::bind(u32 binding, shared_pointer<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
  string assignment = {"0x", hex(device->id()), "/", groupID, "/", inputID};

  if(device->isNull()) {
    return unbind(binding), true;
  }

  if(device->isKeyboard() && device->group(groupID).input(inputID).name() == "Escape") {
    return unbind(binding), true;
  }

  if(device->isKeyboard() && oldValue == 0 && newValue != 0) {
    return bind(binding, assignment), true;
  }

  if(device->isMouse() && oldValue == 0 && newValue != 0) {
    return bind(binding, assignment), true;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Button && oldValue == 0 && newValue != 0) {
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

auto InputDigital::value() -> s16 {
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

    if(device->isMouse() && groupID == HID::Mouse::GroupID::Button && ruby::input.acquired()) {
      output = value != 0;
    }

    if(device->isJoypad() && groupID == HID::Joypad::GroupID::Button) {
      output = value != 0;
    }

    if(device->isJoypad() && groupID != HID::Joypad::GroupID::Button) {
      if(qualifier == Qualifier::Lo) output = value < -16384;
      if(qualifier == Qualifier::Hi) output = value > +16384;
    }

    result |= output;
  }

  return result;
}

auto InputDigital::pressed() -> bool {
  return value() != 0;
}

//

auto InputAnalog::bind(u32 binding, shared_pointer<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
  string assignment = {"0x", hex(device->id()), "/", groupID, "/", inputID};

  if(device->isNull()) {
    return unbind(binding), true;
  }

  if(device->isKeyboard() && device->group(groupID).input(inputID).name() == "Escape") {
    return unbind(binding), true;
  }

  if(device->isKeyboard() && oldValue == 0 && newValue != 0) {
    return bind(binding, assignment), true;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Button && oldValue == 0 && newValue != 0) {
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

auto InputAnalog::value() -> s16 {
  s32 result = 0;

  for(auto& binding : bindings) {
    if(!binding.device) continue;  //unbound

    auto& device = binding.device;
    auto& groupID = binding.groupID;
    auto& inputID = binding.inputID;
    auto& qualifier = binding.qualifier;
    s16 value = device->group(groupID).input(inputID).value();

    if(device->isKeyboard() && groupID == HID::Keyboard::GroupID::Button) {
      result += value != 0 ? 32767 : 0;
    }

    if(device->isJoypad() && groupID == HID::Joypad::GroupID::Button) {
      result += value != 0 ? 32767 : 0;
    }

    if(device->isJoypad() && groupID != HID::Joypad::GroupID::Button) {
      if(qualifier == Qualifier::Lo && value < 0) result += abs(value);
      if(qualifier == Qualifier::Hi && value > 0) result += abs(value);
    }
  }

  return sclamp<16>(result);
}

auto InputAnalog::pressed() -> bool {
  return value() > 16384;
}

//

auto InputAbsolute::bind(u32 binding, shared_pointer<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
  string assignment = {"0x", hex(device->id()), "/", groupID, "/", inputID};

  if(device->isNull()) {
    return unbind(binding), true;
  }

  if(device->isKeyboard() && device->group(groupID).input(inputID).name() == "Escape") {
    return unbind(binding), true;
  }

  if(device->isMouse() && groupID == HID::Mouse::GroupID::Axis) {
    return bind(binding, assignment), true;
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

auto InputAbsolute::value() -> s16 {
  s32 result = 0;

  for(auto& binding : bindings) {
    if(!binding.device) continue;  //unbound

    auto& device = binding.device;
    auto& groupID = binding.groupID;
    auto& inputID = binding.inputID;
    auto& qualifier = binding.qualifier;
    s16 value = device->group(groupID).input(inputID).value();

    if(device->isMouse() && groupID == HID::Joypad::GroupID::Axis && ruby::input.acquired()) {
      result += value;
    }

    if(device->isJoypad() && groupID == HID::Joypad::GroupID::Axis) {
      result += value;
    }
  }

  return sclamp<16>(result);
}

//

auto InputRelative::bind(u32 binding, shared_pointer<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
  string assignment = {"0x", hex(device->id()), "/", groupID, "/", inputID};

  if(device->isNull()) {
    return unbind(binding), true;
  }

  if(device->isKeyboard() && device->group(groupID).input(inputID).name() == "Escape") {
    return unbind(binding), true;
  }

  if(device->isMouse() && groupID == HID::Mouse::GroupID::Axis) {
    return bind(binding, assignment), true;
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

auto InputRelative::value() -> s16 {
  s32 result = 0;

  for(auto& binding : bindings) {
    if(!binding.device) continue;  //unbound

    auto& device = binding.device;
    auto& groupID = binding.groupID;
    auto& inputID = binding.inputID;
    auto& qualifier = binding.qualifier;
    s16 value = device->group(groupID).input(inputID).value();

    if(device->isMouse() && groupID == HID::Joypad::GroupID::Axis && ruby::input.acquired()) {
      result += value;
    }

    if(device->isJoypad() && groupID == HID::Joypad::GroupID::Axis) {
      result += value;
    }
  }

  return sclamp<16>(result);
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
  InputDevice::name = "Default Gamepad";

  // Digital Pad
  InputDevice::digital("D-Up",    up);
  InputDevice::digital("D-Down",  down);
  InputDevice::digital("D-Left",  left);
  InputDevice::digital("D-Right", right);

  // Option buttons
  InputDevice::digital("Select", select);
  InputDevice::digital("Start",  start);
  InputDevice::digital("Mode",   mode);

  // Bottom Row buttons
  InputDevice::digital("B1", b1);
  InputDevice::digital("B2", b2);
  InputDevice::digital("B3", b3);

  // Top Row buttons
  InputDevice::digital("T1", t1);
  InputDevice::digital("T2", t2);
  InputDevice::digital("T3", t3);

  // Shoulder buttons
  InputDevice::digital("Shoulder-L1", l1);
  InputDevice::digital("Shoulder-R1", r1);
  InputDevice::digital("Shoulder-L2", l2);
  InputDevice::digital("Shoulder-R2", r2);

  // Trigger
  InputDevice::digital("Trigger-Left",  lt);
  InputDevice::digital("Trigger-Right", rt);

  // Analog joystick
  InputDevice::analog("LAnalog-Up",     lup);
  InputDevice::analog("LAnalog-Down",   ldown);
  InputDevice::analog("LAnalog-Left",   lleft);
  InputDevice::analog("LAnalog-Right",  lright);
  InputDevice::digital("LAnalog-Thumb", thumbl); // Required for Dual Shock support
  InputDevice::analog("RAnalog-Up",     rup);
  InputDevice::analog("RAnalog-Down",   rdown);
  InputDevice::analog("RAnalog-Left",   rleft);
  InputDevice::analog("RAnalog-Right",  rright);
  InputDevice::digital("RAnalog-Thumb", thumbr); // Required for Dual Shock support

  // NumPad
  InputDevice::digital("Numpad-0",        zero);
  InputDevice::digital("Numpad-1",        one);
  InputDevice::digital("Numpad-2",        two);
  InputDevice::digital("Numpad-3",        three);
  InputDevice::digital("Numpad-4",        four);
  InputDevice::digital("Numpad-5",        five);
  InputDevice::digital("Numpad-6",        six);
  InputDevice::digital("Numpad-7",        seven);
  InputDevice::digital("Numpad-8",        eight);
  InputDevice::digital("Numpad-9",        nine);
  InputDevice::digital("Numpad-Asterisk", asterisk);
  InputDevice::digital("Numpad-Pound",    pound);

  // Rumble
  InputDevice::rumble("Rumble", rumble);                        
}

//

VirtualMouse::VirtualMouse() {
  InputDevice::name = "Mouse";
  InputDevice::relative("X",      x);
  InputDevice::relative("Y",      y);
  InputDevice::digital ("Left",   left);
  InputDevice::digital ("Middle", middle);
  InputDevice::digital ("Right",  right);
}

//

auto InputManager::create() -> void {
  createHotkeys();
}

auto InputManager::bind() -> void {
  for(auto& port : virtualPorts) {
    for(auto& input : port.pad.inputs) input.mapping->bind();
    for(auto& input : port.mouse.inputs) input.mapping->bind();
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
