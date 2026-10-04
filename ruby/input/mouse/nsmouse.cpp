#pragma once

struct InputMouseNS {
  Input& input;
  InputMouseNS(Input& input) : input(input) {}
  NSPoint previousLocation = {0,0};
  std::shared_ptr<HID::Mouse> hid = std::make_shared<HID::Mouse>();
  bool isAcquired = false;

  auto acquired() -> bool {
    return isAcquired;
  }

  auto acquire() -> bool {
    [NSCursor hide];
    previousLocation = [NSEvent mouseLocation];
    isAcquired = true;
    return acquired();
  }

  auto release() -> bool {
    [NSCursor unhide];
    isAcquired = false;
    return true;
  }

  auto assign(u32 groupID, u32 inputID, s16 value) -> void {
    auto& group = hid->group(groupID);
    auto& target = group.input(inputID);
    if(target.value() != value) {
      input.doChange(hid, groupID, inputID, target.value(), value);
      target.setValue(value);
    }
    if(groupID == HID::Mouse::GroupID::Axis) hid->motion(inputID).move(value);
  }
    
  auto poll(std::vector<std::shared_ptr<HID::Device>>& devices) -> void {
    NSUInteger mouseButtons = [NSEvent pressedMouseButtons];
    NSPoint mouseLocation = [NSEvent mouseLocation];
    float deltaX = (previousLocation.x - mouseLocation.x) * -1;
    float deltaY = (previousLocation.y - mouseLocation.y);

    assign(HID::Mouse::GroupID::Button, 0, mouseButtons & 0x1);
    assign(HID::Mouse::GroupID::Button, 1, mouseButtons & 0x4);
    assign(HID::Mouse::GroupID::Button, 2, mouseButtons & 0x2);
    assign(HID::Mouse::GroupID::Axis, 0, deltaX);
    assign(HID::Mouse::GroupID::Axis, 1, deltaY);

    devices.push_back(hid);
    previousLocation = mouseLocation;
  }


  auto initialize(uintptr handle) -> bool {
    hid->setVendorID(HID::Mouse::GenericVendorID);
    hid->setProductID(HID::Mouse::GenericProductID);
    hid->setPathID(0);

    hid->appendAxis("X");
    hid->appendAxis("Y");

    hid->buttons().append("Left");
    hid->buttons().append("Middle");
    hid->buttons().append("Right");

    return true;
  }

  auto terminate() -> void {}
};
