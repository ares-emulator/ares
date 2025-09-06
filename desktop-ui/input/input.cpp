#include "../desktop-ui.hpp"
#include "hotkeys.cpp"

VirtualPort virtualPorts[5];
InputManager inputManager;

auto InputMapping::bind() -> void {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
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

auto InputMapping::Binding::icon() -> multiFactorImage {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  if(!device && deviceID) return Icon::Device::Joypad;
  if(!device) return {};
  if(device->isKeyboard()) return Icon::Device::Keyboard;
  if(device->isMouse()) return Icon::Device::Mouse;
  if(device->isJoypad()) return Icon::Device::Joypad;
  return {};
}

auto InputMapping::Binding::text() -> string {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
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
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  string assignment = {"0x", hex(device->id()), "/", groupID, "/", inputID};

  if(device->isNull()) {
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

  for(auto& binding : bindings) {
    if(!binding.device) continue;  //unbound

    auto& device = binding.device;
    auto& groupID = binding.groupID;
    auto& inputID = binding.inputID;
    auto& qualifier = binding.qualifier;
    if (device->isKeyboard() && program.keyboardCaptured) continue;
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
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  return value() != 0;
}


auto InputHotkey::value() -> s16 {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
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


//

auto InputAnalog::bind(u32 binding, shared_pointer<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
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
    if(!binding.device) continue;  //unbound

    auto& device = binding.device;
    auto& groupID = binding.groupID;
    auto& inputID = binding.inputID;
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

auto InputAbsolute::bind(u32 binding, shared_pointer<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
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
    if(!binding.device) continue;  //unbound

    auto& device = binding.device;
    auto& groupID = binding.groupID;
    auto& inputID = binding.inputID;
    auto& qualifier = binding.qualifier;
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

auto InputRelative::bind(u32 binding, shared_pointer<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
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
    if(!binding.device) continue;  //unbound

    auto& device = binding.device;
    auto& groupID = binding.groupID;
    auto& inputID = binding.inputID;
    auto& qualifier = binding.qualifier;
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

auto InputRumble::bind(u32 binding, shared_pointer<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool {
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
    if(!binding.device) continue;
    ruby::input.rumble(binding.deviceID, strong, weak);
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

VirtualKeyboard::VirtualKeyboard() {
  InputDevice::name = "Keyboard";

  InputDevice::digital("Escape",        esc);
  InputDevice::digital("F1",            f1);
  InputDevice::digital("F2",            f2);
  InputDevice::digital("F3",            f3);
  InputDevice::digital("F4",            f4);
  InputDevice::digital("F5",            f5);
  InputDevice::digital("F6",            f6);
  InputDevice::digital("F7",            f7);
  InputDevice::digital("F8",            f8);
  InputDevice::digital("F9",            f9);
  InputDevice::digital("F10",           f10);
  InputDevice::digital("F11",           f11);
  InputDevice::digital("F12",           f12);

  InputDevice::digital("Print Screen",  print_screen);
  InputDevice::digital("Scroll Lock",   scroll_lock);
  //InputDevice::digital("Pause",         pause);

  InputDevice::digital("Tilde",         tilde);
  InputDevice::digital("Num1",          num1);
  InputDevice::digital("Num2",          num2);
  InputDevice::digital("Num3",          num3);
  InputDevice::digital("Num4",          num4);
  InputDevice::digital("Num5",          num5);
  InputDevice::digital("Num6",          num6);
  InputDevice::digital("Num7",          num7);
  InputDevice::digital("Num8",          num8);
  InputDevice::digital("Num9",          num9);
  InputDevice::digital("Num0",          num0);
  InputDevice::digital("Dash",          dash);
  InputDevice::digital("Equals",        equals);
  InputDevice::digital("Backspace",     backspace);

  InputDevice::digital("Tab",           tab);
  InputDevice::digital("Q",             q);
  InputDevice::digital("W",             w);
  InputDevice::digital("E",             e);
  InputDevice::digital("R",             r);
  InputDevice::digital("T",             t);
  InputDevice::digital("Y",             y);
  InputDevice::digital("U",             u);
  InputDevice::digital("I",             i);
  InputDevice::digital("O",             o);
  InputDevice::digital("P",             p);
  InputDevice::digital("Left Bracket",  lbracket);
  InputDevice::digital("Right Bracket", rbracket);
  InputDevice::digital("Backslash",     backslash);

  InputDevice::digital("Insert",        insert);
  InputDevice::digital("Delete",        delete_);
  InputDevice::digital("Home",          home);
  InputDevice::digital("End",           end);
  InputDevice::digital("Page Up",       page_up);
  InputDevice::digital("Page Down",     page_down);

  InputDevice::digital("Caps Lock",     capslock);
  InputDevice::digital("A",             a);
  InputDevice::digital("S",             s);
  InputDevice::digital("D",             d);
  InputDevice::digital("F",             f);
  InputDevice::digital("G",             g);
  InputDevice::digital("H",             h);
  InputDevice::digital("J",             j);
  InputDevice::digital("K",             k);
  InputDevice::digital("L",             l);
  InputDevice::digital("Semicolon",     semicolon);
  InputDevice::digital("Quote",         apostrophe);
  InputDevice::digital("Return",        return_);

  InputDevice::digital("Left Shift",    lshift);
  InputDevice::digital("Z",             z);
  InputDevice::digital("X",             x);
  InputDevice::digital("C",             c);
  InputDevice::digital("V",             v);
  InputDevice::digital("B",             b);
  InputDevice::digital("N",             n);
  InputDevice::digital("M",             m);
  InputDevice::digital("Comma",         comma);
  InputDevice::digital("Period",        period);
  InputDevice::digital("Slash",         slash);
  InputDevice::digital("Right Shift",   rshift);

  InputDevice::digital("Left Control",  lctrl);
  //InputDevice::digital("Left Super",    lsuper);
  InputDevice::digital("Left Alt",      lalt);
  InputDevice::digital("SpaceBar",      spacebar);
  InputDevice::digital("Right Alt",     ralt);
  //InputDevice::digital("Right Super",   rsuper);
  InputDevice::digital("Right Control", rctrl);

  InputDevice::digital("Up",            up);
  InputDevice::digital("Down",          down);
  InputDevice::digital("Left",          left);
  InputDevice::digital("Right",         right);

  //InputDevice::digital("Num Lock",      numlock);
  InputDevice::digital("Divide",        divide);
  InputDevice::digital("Multiply",      multiply);
  InputDevice::digital("Minus",         minus);

  InputDevice::digital("Keypad7",       num7);
  InputDevice::digital("Keypad8",       num8);
  InputDevice::digital("Keypad9",       num9);
  InputDevice::digital("Add",           add);

  InputDevice::digital("Keypad4",       num4);
  InputDevice::digital("Keypad5",       num5);
  InputDevice::digital("Keypad6",       num6);

  InputDevice::digital("Keypad1",       num1);
  InputDevice::digital("Keypad2",       num2);
  InputDevice::digital("Keypad3",       num3);
  InputDevice::digital("Enter",         enter);

  InputDevice::digital("Keypad0",       num0);
  InputDevice::digital("Point",         point);
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
    for(auto& input : port.keyboard.inputs) input.mapping->bind();
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

auto InputManager::eventInput(shared_pointer<HID::Device> device, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> void {
  lock_guard<recursive_mutex> inputLock(program.inputMutex);
  inputSettings.eventInput(device, groupID, inputID, oldValue, newValue);
  hotkeySettings.eventInput(device, groupID, inputID, oldValue, newValue);
}
