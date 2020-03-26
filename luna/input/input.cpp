#include "../luna.hpp"
#include "hotkeys.cpp"

InputManager inputManager;
Hotkeys& hotkeys = inputManager.hotkeys;

auto InputButton::value() -> int16_t {
  auto value = device->group(groupID).input(inputID).value();

  if(device->isKeyboard() && groupID == HID::Keyboard::Button) {
    return value == 1;
  }

  if(device->isJoypad() && (groupID == HID::Joypad::Axis || groupID == HID::Joypad::Hat)) {
    if(qualifier == Qualifier::Lo && value < -16384) return 1;
    if(qualifier == Qualifier::Hi && value > +16384) return 1;
    return 0;
  }

  if(device->isJoypad() && groupID == HID::Joypad::Button) {
    return value == 1;
  }

  return 0;
}

//

auto InputAxis::value() -> int16_t {
  if(!inputInstance.acquired()) return 0;
  auto value = device->group(groupID).input(inputID).value();

  if(device->isMouse() && groupID == HID::Mouse::Axis) {
    return value;
  }

  return 0;
}

//

auto InputManager::bind(maybe<ares::Node::Object> newRoot) -> void {
  if(newRoot) root = newRoot();
  if(!root) return;

  for(auto& input : root->find<ares::Node::Input>()) {
    //not strictly necessary; but release any shared_pointer instances to free up the allocated memory
    input->setAttribute<shared_pointer<InputButton>>("instance");

    auto _pathID = input->attribute("pathID"); if(!_pathID) continue;
    auto _vendorID = input->attribute("vendorID"); if(!_vendorID) continue;
    auto _productID = input->attribute("productID"); if(!_productID) continue;

    auto pathID = _pathID.natural();
    auto vendorID = _vendorID.natural();
    auto productID = _productID.natural();

    for(auto& device : devices) {
      if(pathID != device->pathID()) continue;
      if(vendorID != device->vendorID()) continue;
      if(productID != device->productID()) continue;

      if(input->cast<ares::Node::Button>()) {
        auto instance = shared_pointer_make<InputButton>();
        instance->device = device;
        instance->groupID = input->attribute("groupID").natural();
        instance->inputID = input->attribute("inputID").natural();
        if(input->attribute("qualifier") == "Lo") instance->qualifier = InputButton::Qualifier::Lo;
        if(input->attribute("qualifier") == "Hi") instance->qualifier = InputButton::Qualifier::Hi;
        input->setAttribute<shared_pointer<InputButton>>("instance", instance);
        break;
      }

      if(input->cast<ares::Node::Axis>()) {
        auto instance = shared_pointer_make<InputAxis>();
        instance->device = device;
        instance->groupID = input->attribute("groupID").natural();
        instance->inputID = input->attribute("inputID").natural();
        input->setAttribute<shared_pointer<InputAxis>>("instance", instance);
        break;
      }
    }
  }
}

auto InputManager::unbind() -> void {
  this->root = {};
}

auto InputManager::poll() -> void {
  //polling actual hardware is very time-consuming: skip call if poll was called too recently
  auto thisPoll = chrono::millisecond();
  if(thisPoll - lastPoll < pollFrequency) return;
  lastPoll = thisPoll;

  //poll hardware, detect when the available devices have changed:
  //existing in-use devices may have been disconnected; or mapped but disconnected devices may now be available.
  //as such, when the returned devices tree changes, rebind all inputs
  auto devices = inputInstance.poll();
  bool changed = devices.size() != this->devices.size();
  if(!changed) {
    for(uint n : range(devices.size())) {
      if(changed = devices[n] != this->devices[n]) break;
    }
  }
  if(changed) {
    this->devices = devices;
    bind();
  }
}

auto InputManager::eventInput(shared_pointer<HID::Device> device, uint group, uint input, int16_t oldValue, int16_t newValue) -> void {
  inputMapper.eventInput(device, group, input, oldValue, newValue);
  hotkeySettings.eventInput(device, group, input, oldValue, newValue);
}
