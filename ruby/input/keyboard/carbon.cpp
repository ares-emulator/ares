#pragma once

struct InputKeyboardCarbon {
  Input& input;
  InputKeyboardCarbon(Input& input) : input(input) {}

  shared_pointer<HID::Keyboard> hid{new HID::Keyboard};

  struct Key {
    u8 id = 0;
    string name;
  };
  std::vector<Key> keys;

  auto assign(u32 inputID, bool value) -> void {
    auto& group = hid->buttons();
    if(group.input(inputID).value() == value) return;
    input.doChange(hid, HID::Keyboard::GroupID::Button, inputID, group.input(inputID).value(), value);
    group.input(inputID).setValue(value);
  }

  auto poll(std::vector<shared_pointer<HID::Device>>& devices) -> void {
    KeyMap keymap;
    GetKeys(keymap);
    auto buffer = (const uint8*)keymap;

    u32 inputID = 0;
    for(auto& key : keys) {
      bool value = buffer[key.id >> 3] & (1 << (key.id & 7));
      assign(inputID++, value);
    }

    devices.push_back(hid);
  }

  auto initialize() -> bool {
    keys.push_back({0x35, "Escape"});
    keys.push_back({0x7a, "F1"});
    keys.push_back({0x78, "F2"});
    keys.push_back({0x63, "F3"});
    keys.push_back({0x76, "F4"});
    keys.push_back({0x60, "F5"});
    keys.push_back({0x61, "F6"});
    keys.push_back({0x62, "F7"});
    keys.push_back({0x64, "F8"});
    keys.push_back({0x65, "F9"});
    keys.push_back({0x6d, "F10"});
    keys.push_back({0x67, "F11"});
  //keys.push_back({0x??, "F12"});

    keys.push_back({0x69, "PrintScreen"});
  //keys.push_back({0x??, "ScrollLock"});
    keys.push_back({0x71, "Pause"});

    keys.push_back({0x32, "Tilde"});
    keys.push_back({0x12, "Num1"});
    keys.push_back({0x13, "Num2"});
    keys.push_back({0x14, "Num3"});
    keys.push_back({0x15, "Num4"});
    keys.push_back({0x17, "Num5"});
    keys.push_back({0x16, "Num6"});
    keys.push_back({0x1a, "Num7"});
    keys.push_back({0x1c, "Num8"});
    keys.push_back({0x19, "Num9"});
    keys.push_back({0x1d, "Num0"});

    keys.push_back({0x1b, "Dash"});
    keys.push_back({0x18, "Equal"});
    keys.push_back({0x33, "Backspace"});

    keys.push_back({0x72, "Insert"});
    keys.push_back({0x75, "Delete"});
    keys.push_back({0x73, "Home"});
    keys.push_back({0x77, "End"});
    keys.push_back({0x74, "PageUp"});
    keys.push_back({0x79, "PageDown"});

    keys.push_back({0x00, "A"});
    keys.push_back({0x0b, "B"});
    keys.push_back({0x08, "C"});
    keys.push_back({0x02, "D"});
    keys.push_back({0x0e, "E"});
    keys.push_back({0x03, "F"});
    keys.push_back({0x05, "G"});
    keys.push_back({0x04, "H"});
    keys.push_back({0x22, "I"});
    keys.push_back({0x26, "J"});
    keys.push_back({0x28, "K"});
    keys.push_back({0x25, "L"});
    keys.push_back({0x2e, "M"});
    keys.push_back({0x2d, "N"});
    keys.push_back({0x1f, "O"});
    keys.push_back({0x23, "P"});
    keys.push_back({0x0c, "Q"});
    keys.push_back({0x0f, "R"});
    keys.push_back({0x01, "S"});
    keys.push_back({0x11, "T"});
    keys.push_back({0x20, "U"});
    keys.push_back({0x09, "V"});
    keys.push_back({0x0d, "W"});
    keys.push_back({0x07, "X"});
    keys.push_back({0x10, "Y"});
    keys.push_back({0x06, "Z"});

    keys.push_back({0x21, "LeftBracket"});
    keys.push_back({0x1e, "RightBracket"});
    keys.push_back({0x2a, "Backslash"});
    keys.push_back({0x29, "Semicolon"});
    keys.push_back({0x27, "Apostrophe"});
    keys.push_back({0x2b, "Comma"});
    keys.push_back({0x2f, "Period"});
    keys.push_back({0x2c, "Slash"});

    keys.push_back({0x53, "Keypad1"});
    keys.push_back({0x54, "Keypad2"});
    keys.push_back({0x55, "Keypad3"});
    keys.push_back({0x56, "Keypad4"});
    keys.push_back({0x57, "Keypad5"});
    keys.push_back({0x58, "Keypad6"});
    keys.push_back({0x59, "Keypad7"});
    keys.push_back({0x5b, "Keypad8"});
    keys.push_back({0x5c, "Keypad9"});
    keys.push_back({0x52, "Keypad0"});

  //keys.push_back({0x??, "Point"});
    keys.push_back({0x45, "Add"});
    keys.push_back({0x4e, "Subtract"});
    keys.push_back({0x43, "Multiply"});
    keys.push_back({0x4b, "Divide"});
    keys.push_back({0x4c, "Enter"});

    keys.push_back({0x47, "NumLock"});
  //keys.push_back({0x39, "CapsLock"});

    keys.push_back({0x7e, "Up"});
    keys.push_back({0x7d, "Down"});
    keys.push_back({0x7b, "Left"});
    keys.push_back({0x7c, "Right"});

    keys.push_back({0x30, "Tab"});
    keys.push_back({0x24, "Return"});
    keys.push_back({0x31, "Spacebar"});
  //keys.push_back({0x??, "Menu"});

    keys.push_back({0x38, "Shift"});
    keys.push_back({0x3b, "Control"});
    keys.push_back({0x3a, "Alt"});
    keys.push_back({0x37, "Super"});

    hid->setVendorID(HID::Keyboard::GenericVendorID);
    hid->setProductID(HID::Keyboard::GenericProductID);
    hid->setPathID(0);
    for(auto& key : keys) {
      hid->buttons().append(key.name);
    }

    return true;
  }

  auto terminate() -> void {
  }
};
