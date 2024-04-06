#pragma once

struct InputMouseGC {
  Input& input;
  InputMouseGC(Input& input) : input(input) {}

  uintptr handle = 0;

  shared_pointer<HID::Mouse> hid{new HID::Mouse};

  auto acquired() -> bool {
    return true;
  }

  auto acquire() -> bool {
    // ms.mouse = [GCMouse current];
    return acquired();
  }

  auto release() -> bool {
    return true;
  }

  auto assign(u32 groupID, u32 inputID, s16 value) -> void {
    auto& group = hid->group(groupID);
    if(group.input(inputID).value() == value) return;
    input.doChange(hid, groupID, inputID, group.input(inputID).value(), value);
    group.input(inputID).setValue(value);
  }
    
  auto poll(vector<shared_pointer<HID::Device>>& devices) -> void {
    NSUInteger mouseButtons = [NSEvent pressedMouseButtons];
    NSPoint mouseLocation = [NSEvent mouseLocation];
    NSWindow* window = [NSApp mainWindow];
    NSPoint windowLocation = [window convertPointFromScreen:mouseLocation];

    assign(HID::Mouse::GroupID::Button, 0, mouseButtons & 0x1);
    assign(HID::Mouse::GroupID::Button, 1, mouseButtons & 0x4);
    assign(HID::Mouse::GroupID::Button, 2, mouseButtons & 0x2);

    assign(HID::Mouse::GroupID::Axis, 0, windowLocation.x);
    assign(HID::Mouse::GroupID::Axis, 1, windowLocation.y);

    NSLog(@"Mouse Location - X: %f, Y: %f", windowLocation.x, windowLocation.y);

    devices.append(hid);
  }


  auto initialize(uintptr handle) -> bool {
    hid->setVendorID(HID::Mouse::GenericVendorID);
    hid->setProductID(HID::Mouse::GenericProductID);
    hid->setPathID(0);

    hid->axes().append("X");
    hid->axes().append("Y");
    // hid->axes().append("Z");

    hid->buttons().append("Left");
    hid->buttons().append("Middle");
    hid->buttons().append("Right");
    hid->buttons().append("Up");
    hid->buttons().append("Down");

    return true;
  }

  auto terminate() -> void {}
};
