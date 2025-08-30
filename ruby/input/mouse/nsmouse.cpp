#pragma once

struct InputMouseNS {
  Input& input;
  InputMouseNS(Input& input) : input(input) {}
  NSPoint previousLocation = {0,0};
  shared_pointer<HID::Mouse> hid{new HID::Mouse};
  bool isAcquired = false;

  auto acquired() -> bool {
    return isAcquired;
  }

  auto acquire() -> bool {
    [NSCursor hide];
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
    if(group.input(inputID).value() == value) return;
    input.doChange(hid, groupID, inputID, group.input(inputID).value(), value);
    group.input(inputID).setValue(value);
  }
    
  auto poll(std::vector<shared_pointer<HID::Device>>& devices) -> void {
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

    hid->axes().append("X");
    hid->axes().append("Y");

    hid->buttons().append("Left");
    hid->buttons().append("Middle");
    hid->buttons().append("Right");

    return true;
  }

  auto terminate() -> void {}
};
