#include "../desktop-ui.hpp"
#include "hotkeys.cpp"

VirtualPort virtualPorts[5];
InputManager inputManager;

static auto lookupDevice(u64 deviceID) -> std::shared_ptr<HID::Device> {
  for(auto& device : inputManager.devices) {
    if(deviceID == device->id()) return device;
  }
  return {};
}

static auto parseBindingChord(string_view assignment, InputMapping::Qualifier& qualifier, std::vector<InputMapping::Binding::Chord>& chord) -> bool {
  if(assignment.size() == 0) return false;

  auto segments = nall::split(assignment, "+");
  if(segments.empty()) return false;

  qualifier = InputMapping::Qualifier::None;
  chord.clear();
  chord.reserve(segments.size());

  for(auto& rawSegment : segments) {
    auto segment = string{rawSegment}.strip();
    if(segment.size() == 0) continue;

    auto token = nall::split(segment, "/");
    if(token.size() < 3) {
      chord.clear();
      return false;
    }

    InputMapping::Binding::Chord key;
    key.deviceID = token[0].natural();
    key.groupID = token[1].natural();
    key.inputID = token[2].natural();
    key.device = lookupDevice(key.deviceID);
    chord.push_back(key);

    if(chord.size() == 1 && token.size() > 3) {
      if(token[3] == "Lo") qualifier = InputMapping::Qualifier::Lo;
      if(token[3] == "Hi") qualifier = InputMapping::Qualifier::Hi;
      if(token[3] == "Rumble") qualifier = InputMapping::Qualifier::Rumble;
    }
  }

  return !chord.empty();
}

static auto chordPressed(const InputMapping::Binding::Chord& key, InputMapping::Qualifier qualifier, bool suppressKeyboardWhenCaptured) -> bool {
  if(!key.device) return false;

  auto& device = key.device;
  auto groupID = key.groupID;
  auto inputID = key.inputID;

  if(suppressKeyboardWhenCaptured && device->isKeyboard() && program.keyboardCaptured) return false;

  s16 value = device->group(groupID).input(inputID).value();
  if(device->isKeyboard() && groupID == HID::Keyboard::GroupID::Button) return value != 0;
  if(device->isMouse() && groupID == HID::Mouse::GroupID::Button && ruby::input.acquired()) return value != 0;
  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Button) return value != 0;

  if(device->isJoypad() && groupID != HID::Joypad::GroupID::Button) {
    if(qualifier == InputMapping::Qualifier::Lo) return value < -16384;
    if(qualifier == InputMapping::Qualifier::Hi) return value > +16384;
  }

  return false;
}

static auto bindingActive(const InputMapping::Binding& binding, bool suppressKeyboardWhenCaptured) -> bool {
  if(binding.chord.empty()) return false;
  if(binding.chord.size() == 1) return chordPressed(binding.chord[0], binding.qualifier, suppressKeyboardWhenCaptured);
  for(auto index : range(binding.chord.size())) {
    auto qualifier = index == 0 ? binding.qualifier : InputMapping::Qualifier::None;
    if(!chordPressed(binding.chord[index], qualifier, suppressKeyboardWhenCaptured)) return false;
  }
  return true;
}

auto InputMapping::bind() -> void {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  for(auto& binding : bindings) binding = {};

  for(u32 index : range(BindingLimit)) {
    auto& assignment = assignments[index];
    auto& binding = bindings[index];
    parseBindingChord(assignment, binding.qualifier, binding.chord);
  }
}

auto InputMapping::bind(u32 binding, string assignment) -> void {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  if(binding >= BindingLimit) return;
  assignments[binding] = assignment;
  bind();
}

auto InputMapping::unbind() -> void {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  for(u32 binding : range(BindingLimit)) unbind(binding);
}

auto InputMapping::unbind(u32 binding) -> void {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  if(binding >= BindingLimit) return;
  bindings[binding] = {};
  assignments[binding] = {};
}

auto InputMapping::Binding::primary() const -> const Chord* {
  if(chord.empty()) return nullptr;
  return &chord[0];
}

auto InputMapping::Binding::icon() -> multiFactorImage {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  auto key = primary();
  if(!key) return {};
  if(!key->device && key->deviceID) return Icon::Device::Joypad;
  if(!key->device) return {};
  if(key->device->isKeyboard()) return Icon::Device::Keyboard;
  if(key->device->isMouse()) return Icon::Device::Mouse;
  if(key->device->isJoypad()) return Icon::Device::Joypad;
  return {};
}

auto InputMapping::Binding::text() -> string {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  if(chord.empty()) return {};

  if(chord.size() > 1) {
    string output;
    for(auto index : range(chord.size())) {
      auto& key = chord[index];
      if(!key.device && key.deviceID) return "(disconnected)";
      if(!key.device) return {};
      if(key.groupID >= key.device->size()) return {};
      if(key.inputID >= key.device->group(key.groupID).size()) return {};
      if(index) output.append("+");
      output.append(key.device->group(key.groupID).input(key.inputID).name());
    }
    return output;
  }

  auto& key = chord[0];
  if(!key.device && key.deviceID) return "(disconnected)";
  if(!key.device) return {};
  if(key.groupID >= key.device->size()) return {};
  if(key.inputID >= key.device->group(key.groupID).size()) return {};

  if(key.device->isKeyboard()) {
    return key.device->group(key.groupID).input(key.inputID).name();
  }

  if(key.device->isMouse()) {
    return key.device->group(key.groupID).input(key.inputID).name();
  }

  if(key.device->isJoypad()) {
    string name = key.device->name();
    if(name == "Joypad") {
      name.append(string{"{", Hash::CRC16(string{key.device->id()}).digest().upcase(), "}"});
    }
    name.append(" ", key.device->group(key.groupID).name());
    name.append(" ", key.device->group(key.groupID).input(key.inputID).name());
    if(qualifier == Qualifier::Lo) name.append(".Lo");
    if(qualifier == Qualifier::Hi) name.append(".Hi");
    if(qualifier == Qualifier::Rumble) name.append(".Rumble");
    return name;
  }

  return {};
}

//

auto InputDigital::bind(u32 binding, std::shared_ptr<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
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

  if(device->isJoypad() && groupID != HID::Joypad::GroupID::Button
  && oldValue >= -16384 && newValue < -16384
  ) {
    return bind(binding, {assignment, "/Lo"}), true;
  }

  if(device->isJoypad() && groupID != HID::Joypad::GroupID::Button
  && oldValue <= +16384 && newValue > +16384
  ) {
    return bind(binding, {assignment, "/Hi"}), true;
  }

  return false;
}

auto InputDigital::value() -> s16 {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  s16 result = 0;
  for(auto& binding : bindings) result |= bindingActive(binding, true);
  return result;
}

auto InputDigital::pressed() -> bool {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  return value() != 0;
}


auto InputHotkey::value() -> s16 {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  s16 result = 0;
  for(auto& binding : bindings) result |= bindingActive(binding, false);
  return result;
}


//

auto InputAnalog::bind(u32 binding, std::shared_ptr<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
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

  if(device->isJoypad() && groupID != HID::Joypad::GroupID::Button
  && oldValue >= -16384 && newValue < -16384
  ) {
    return bind(binding, {assignment, "/Lo"}), true;
  }

  if(device->isJoypad() && groupID != HID::Joypad::GroupID::Button
  && oldValue <= +16384 && newValue > +16384
  ) {
    return bind(binding, {assignment, "/Hi"}), true;
  }

  return false;
}

auto InputAnalog::value() -> s16 {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  s32 result = 0;

  for(auto& binding : bindings) {
    auto key = binding.primary();
    if(!key || !key->device) continue;  //unbound

    auto& device = key->device;
    auto& groupID = key->groupID;
    auto& inputID = key->inputID;
    auto& qualifier = binding.qualifier;
    if (device->isKeyboard() && program.keyboardCaptured) continue;    
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
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  return value() > 16384;
}

//

auto InputAbsolute::bind(u32 binding, std::shared_ptr<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
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

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Axis
  && oldValue >= -16384 && newValue < -16384
  ) {
    return bind(binding, assignment), true;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Axis
  && oldValue <= +16384 && newValue > +16384
  ) {
    return bind(binding, assignment), true;
  }

  return false;
}

auto InputAbsolute::value() -> s16 {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  s32 result = 0;

  for(auto& binding : bindings) {
    auto key = binding.primary();
    if(!key || !key->device) continue;  //unbound

    auto& device = key->device;
    auto& groupID = key->groupID;
    auto& inputID = key->inputID;
    if (device->isKeyboard() && program.keyboardCaptured) continue;
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

auto InputRelative::bind(u32 binding, std::shared_ptr<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
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

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Axis
  && oldValue >= -16384 && newValue < -16384
  ) {
    return bind(binding, assignment), true;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Axis
  && oldValue <= +16384 && newValue > +16384
  ) {
    return bind(binding, assignment), true;
  }

  return false;
}

auto InputRelative::value() -> s16 {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  s32 result = 0;

  for(auto& binding : bindings) {
    auto key = binding.primary();
    if(!key || !key->device) continue;  //unbound

    auto& device = key->device;
    auto& groupID = key->groupID;
    auto& inputID = key->inputID;
    if (device->isKeyboard() && program.keyboardCaptured) continue;
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

auto InputRumble::bind(u32 binding, std::shared_ptr<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
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

auto InputRumble::rumble(u16 strong, u16 weak) -> void {
  for(auto& binding : bindings) {
    auto key = binding.primary();
    if(!key || !key->device) continue;
    ruby::input.rumble(key->deviceID, strong, weak);
  }
}

//

VirtualPad::VirtualPad() {
  InputDevice::name = "Virtual Gamepad";
  InputDevice::digital("Pad Up",          up);
  InputDevice::digital("Pad Down",        down);
  InputDevice::digital("Pad Left",        left);
  InputDevice::digital("Pad Right",       right);
  InputDevice::digital("Select",          select);
  InputDevice::digital("Start",           start);
  InputDevice::digital("A (South)",       south);
  InputDevice::digital("B (East)",        east);
  InputDevice::digital("X (West)",        west);
  InputDevice::digital("Y (North)",       north);
  InputDevice::digital("L-Bumper",        l_bumper);
  InputDevice::digital("R-Bumper",        r_bumper);
  InputDevice::digital("L-Trigger",       l_trigger);
  InputDevice::digital("R-Trigger",       r_trigger);
  InputDevice::digital("L-Stick (Click)", lstick_click);
  InputDevice::digital("R-Stick (Click)", rstick_click);
  InputDevice::analog ("L-Up",            lstick_up);
  InputDevice::analog ("L-Down",          lstick_down);
  InputDevice::analog ("L-Left",          lstick_left);
  InputDevice::analog ("L-Right",         lstick_right);
  InputDevice::analog ("R-Up",            rstick_up);
  InputDevice::analog ("R-Down",          rstick_down);
  InputDevice::analog ("R-Left",          rstick_left);
  InputDevice::analog ("R-Right",         rstick_right);
  InputDevice::rumble ("Rumble",          rumble);
}

//

VirtualMouse::VirtualMouse() {
  InputDevice::name = "Mouse";
  InputDevice::relative("X",      x);
  InputDevice::relative("Y",      y);
  InputDevice::digital ("Left",   left);
  InputDevice::digital ("Middle", middle);
  InputDevice::digital ("Right",  right);
  InputDevice::digital ("Extra",  extra);
}

//

auto InputManager::create() -> void {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  createHotkeys();
}

auto InputManager::bind() -> void {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
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

  lock_guard<recursive_mutex> inputLock(program.inputMutex);
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
    if(settingsWindow.initialized) {
      inputSettings.refresh();
      hotkeySettings.refresh();
    }
  }
}

auto InputManager::eventInput(std::shared_ptr<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> void {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  inputSettings.eventInput(device, groupID, inputID, oldValue, newValue);
  hotkeySettings.eventInput(device, groupID, inputID, oldValue, newValue);
}
