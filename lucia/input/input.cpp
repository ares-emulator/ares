#include "../lucia.hpp"
#include "hotkeys.cpp"

VirtualPad virtualPad;
InputManager inputManager;

auto InputMapping::bind(shared_pointer<HID::Device> device, uint groupID, uint inputID, int16_t oldValue, int16_t newValue) -> bool {
  if(device->isNull()) {
    return unbind(), true;
  }

  if(device->isKeyboard() && device->group(groupID).input(inputID).name() == "Escape") {
    return unbind(), true;
  }

  if(device->isKeyboard() && oldValue == 0 && newValue == 1) {
    return setAssignment(device, groupID, inputID), true;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Button && oldValue == 0 && newValue == 1) {
    return setAssignment(device, groupID, inputID), true;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Hat && newValue < -16384) {
    return setAssignment(device, groupID, inputID, Qualifier::Lo), true;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Hat && newValue > +16384) {
    return setAssignment(device, groupID, inputID, Qualifier::Hi), true;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Axis && newValue < -16384) {
    return setAssignment(device, groupID, inputID, Qualifier::Lo), true;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Axis && newValue > +16384) {
    return setAssignment(device, groupID, inputID, Qualifier::Hi), true;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Trigger && newValue < -16384) {
    return setAssignment(device, groupID, inputID, Qualifier::Lo), true;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Trigger && newValue > +16384) {
    return setAssignment(device, groupID, inputID, Qualifier::Hi), true;
  }

  return false;
}

auto InputMapping::bind() -> void {
  device.reset();
  deviceID = 0;
  groupID = 0;
  inputID = 0;
  qualifier = Qualifier::None;

  if(auto p = assignment.split("/"); p.size() >= 4) {
    for(auto& device : inputManager.devices) {
      if(device->name() == p[0]) {
        if(device->id() == p[1].hex()) {
          if(auto groupID = device->find(p[2])) {
            if(auto inputID = device->group(*groupID).find(p[3])) {
              this->device = device;
              this->deviceID = device->id();
              this->groupID = *groupID;
              this->inputID = *inputID;
              this->qualifier = Qualifier::None;
              if(p.size() >= 5) {
                if(p[4] == "Lo") this->qualifier = Qualifier::Lo;
                if(p[4] == "Hi") this->qualifier = Qualifier::Hi;
              }
              break;
            }
          }
        }
      }
    }
  }
}

auto InputMapping::unbind() -> void {
  device.reset();
  deviceID = 0;
  groupID = 0;
  inputID = 0;
  qualifier = Qualifier::None;
  assignment = "";
}

auto InputMapping::icon() -> image {
  if(assignment.beginsWith("Keyboard/")) return Icon::Device::Keyboard;
  if(assignment.beginsWith("Mouse/")) return Icon::Device::Mouse;
  if(assignment.beginsWith("Joypad/")) return Icon::Device::Joypad;
  return {};
}

auto InputMapping::text() -> string {
  string text = assignment;
  auto part = text.split("/");
  if(text.match("Keyboard/*/Button/*")) return {part[3]};
  if(text.match("Joypad/*/Button/*")) return {part[1], ".Button.", part[3]};
  if(text.match("Joypad/*/Hat/*/*")) return {part[1], ".Hat.", part[3], ".", part[4]};
  if(text.match("Joypad/*/Axis/*/*")) return {part[1], ".Axis.", part[3], ".", part[4]};
  if(text.match("Joypad/*/Trigger/*/*")) return {part[1], ".Trigger.", part[3], ".", part[4]};
  return text;
}

auto InputMapping::value() -> int16_t {
  if(!device) return 0;
  int16_t value = device->group(groupID).input(inputID).value();

  if(device->isKeyboard() && groupID == HID::Keyboard::GroupID::Button) {
    return value != 0;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Button) {
    return value != 0;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Hat) {
    if(qualifier == Qualifier::Lo) return value < -16384;
    if(qualifier == Qualifier::Hi) return value > +16384;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Axis) {
    if(qualifier == Qualifier::Lo) return value < -16384;
    if(qualifier == Qualifier::Hi) return value > +16384;
  }

  if(device->isJoypad() && groupID == HID::Joypad::GroupID::Trigger) {
    if(qualifier == Qualifier::Lo) return value < -16384;
    if(qualifier == Qualifier::Hi) return value > +16384;
  }

  return 0;
}

auto InputMapping::resetAssignment() -> void {
  device.reset();
  deviceID = 0;
  groupID = 0;
  inputID = 0;
  assignment = "";
}

auto InputMapping::setAssignment(shared_pointer<HID::Device> device, uint groupID, uint inputID, Qualifier qualifier) -> void {
  assignment = {device->name(), "/", hex(device->id()), "/", device->group(groupID).name(), "/", device->group(groupID).input(inputID).name()};

  this->device = device;
  this->deviceID = device->id();
  this->groupID = groupID;
  this->inputID = inputID;
  this->qualifier = Qualifier::None;
  if(qualifier == Qualifier::Lo) this->qualifier = Qualifier::Lo, assignment.append("/Lo");
  if(qualifier == Qualifier::Hi) this->qualifier = Qualifier::Hi, assignment.append("/Hi");
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
  mappings.append(&x);
  mappings.append(&y);
  mappings.append(&l);
  mappings.append(&r);
}

//

auto InputManager::create() -> void {
  createHotkeys();
}

auto InputManager::bind() -> void {
  for(auto& mapping : virtualPad.mappings) mapping->bind();
  for(auto& mapping : hotkeys) mapping.bind();
}

auto InputManager::poll() -> void {
  //polling actual hardware is very time-consuming; skip call if poll was called too recently
  auto thisPoll = chrono::millisecond();
  if(thisPoll - lastPoll < pollFrequency) return;
  lastPoll = thisPoll;

  auto devices = ruby::input.poll();
  bool changed = devices.size() != this->devices.size();
  if(!changed) {
    for(uint index : range(devices.size())) {
      changed = devices[index] != this->devices[index];
      if(changed) break;
    }
  }
  if(changed) {
    this->devices = devices;
    bind();
  }
}

auto InputManager::eventInput(shared_pointer<HID::Device> device, uint groupID, uint inputID, int16_t oldValue, int16_t newValue) -> void {
  inputSettings.eventInput(device, groupID, inputID, oldValue, newValue);
  hotkeySettings.eventInput(device, groupID, inputID, oldValue, newValue);
}
