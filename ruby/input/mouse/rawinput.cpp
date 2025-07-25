#pragma once

struct InputMouseRawInput {
  Input& input;
  InputMouseRawInput(Input& input) : input(input) {}

  uintptr handle = 0;
  bool mouseAcquired = false;

  struct Mouse {
    shared_pointer<HID::Mouse> hid{new HID::Mouse};

    s32 relativeX = 0;
    s32 relativeY = 0;
    s32 relativeZ = 0;
    bool buttons[5] = {0};
  } ms;

  auto acquired() -> bool {
    return mouseAcquired;
  }

  auto acquire() -> bool {
    if(!mouseAcquired) {
      mouseAcquired = true;
      ShowCursor(false);
    }
    
    SetFocus((HWND)handle);
    SetCapture((HWND)handle);
    RECT rc;
    GetWindowRect((HWND)handle, &rc);
    ClipCursor(&rc);
    return GetCapture() == (HWND)handle;
  }

  auto release() -> bool {
    if(mouseAcquired) {
      mouseAcquired = false;
      ReleaseCapture();
      ClipCursor(nullptr);
      ShowCursor(true);
    }
    return true;
  }

  auto update(RAWINPUT* input) -> void {
    if((input->data.mouse.usFlags & 1) == MOUSE_MOVE_RELATIVE) {
      ms.relativeX += input->data.mouse.lLastX;
      ms.relativeY += input->data.mouse.lLastY;
    }

    if(input->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
      ms.relativeZ += (s16)input->data.mouse.usButtonData;
    }

    if(input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) ms.buttons[0] = 1;
    if(input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP  ) ms.buttons[0] = 0;
    if(input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) ms.buttons[1] = 1;
    if(input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP  ) ms.buttons[1] = 0;
    if(input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) ms.buttons[2] = 1;
    if(input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP  ) ms.buttons[2] = 0;
    if(input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) ms.buttons[3] = 1;
    if(input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP  ) ms.buttons[3] = 0;
    if(input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) ms.buttons[4] = 1;
    if(input->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP  ) ms.buttons[4] = 0;
  }

  auto assign(u32 groupID, u32 inputID, s16 value) -> void {
    auto& group = ms.hid->group(groupID);
    if(group.input(inputID).value() == value) return;
    input.doChange(ms.hid, groupID, inputID, group.input(inputID).value(), value);
    group.input(inputID).setValue(value);
  }

  auto poll(vector<shared_pointer<HID::Device>>& devices) -> void {
    assign(HID::Mouse::GroupID::Axis, 0, ms.relativeX);
    assign(HID::Mouse::GroupID::Axis, 1, ms.relativeY);
    assign(HID::Mouse::GroupID::Axis, 2, ms.relativeZ);

    //keys are intentionally reordered below:
    //in ruby, button order is {left, middle, right, up, down}
    assign(HID::Mouse::GroupID::Button, 0, ms.buttons[0]);
    assign(HID::Mouse::GroupID::Button, 2, ms.buttons[1]);
    assign(HID::Mouse::GroupID::Button, 1, ms.buttons[2]);
    assign(HID::Mouse::GroupID::Button, 4, ms.buttons[3]);
    assign(HID::Mouse::GroupID::Button, 3, ms.buttons[4]);

    ms.relativeX = 0;
    ms.relativeY = 0;
    ms.relativeZ = 0;

    devices.append(ms.hid);
  }

  auto initialize(uintptr handle) -> bool {
    if(!handle) return false;
    this->handle = handle;

    ms.hid->setVendorID(HID::Mouse::GenericVendorID);
    ms.hid->setProductID(HID::Mouse::GenericProductID);
    ms.hid->setPathID(0);

    ms.hid->axes().append("X");
    ms.hid->axes().append("Y");
    ms.hid->axes().append("Z");

    ms.hid->buttons().append("Left");
    ms.hid->buttons().append("Middle");
    ms.hid->buttons().append("Right");
    ms.hid->buttons().append("Up");
    ms.hid->buttons().append("Down");

    rawinput.updateMouse = {&InputMouseRawInput::update, this};
    return true;
  }

  auto terminate() -> void {
    rawinput.updateMouse.reset();
    release();
  }
};
