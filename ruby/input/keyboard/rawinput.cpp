#pragma once

struct InputKeyboardRawInput {
  Input& input;
  InputKeyboardRawInput(Input& input) : input(input) {}

  struct Key {
    u16 code;
    u16 flag;
    string name;
    bool value;
  };
  std::vector<Key> keys;

  struct Keyboard {
    shared_pointer<HID::Keyboard> hid{new HID::Keyboard};
  } kb;

  auto update(RAWINPUT* input) -> void {
    u32 code = input->data.keyboard.MakeCode;
    u32 flag = input->data.keyboard.Flags;

    for(auto& key : keys) {
      if(key.code != code || key.flag != (flag & ~1)) continue;
      key.value = ~flag & 1;
    }
  }

  auto assign(u32 inputID, bool value) -> void {
    auto& group = kb.hid->buttons();
    if(group.input(inputID).value() == value) return;
    input.doChange(kb.hid, HID::Keyboard::GroupID::Button, inputID, group.input(inputID).value(), value);
    group.input(inputID).setValue(value);
  }

  auto poll(std::vector<shared_pointer<HID::Device>>& devices) -> void {
    for(auto n : range(keys.size())) assign(n, keys[n].value);
    devices.push_back(kb.hid);
  }

  auto initialize() -> bool {
    rawinput.updateKeyboard = {&InputKeyboardRawInput::update, this};

    //Pause sends 0x001d,4 + 0x0045,0; NumLock sends only 0x0045,0
    //pressing Pause will falsely trigger NumLock
    //further, pause sends its key release even while button is held down
    //because of this, we cannot map either reliably

    keys.push_back({0x0001, 0, "Escape"});
    keys.push_back({0x003b, 0, "F1"});
    keys.push_back({0x003c, 0, "F2"});
    keys.push_back({0x003d, 0, "F3"});
    keys.push_back({0x003e, 0, "F4"});
    keys.push_back({0x003f, 0, "F5"});
    keys.push_back({0x0040, 0, "F6"});
    keys.push_back({0x0041, 0, "F7"});
    keys.push_back({0x0042, 0, "F8"});
    keys.push_back({0x0043, 0, "F9"});
    keys.push_back({0x0044, 0, "F10"});
    keys.push_back({0x0057, 0, "F11"});
    keys.push_back({0x0058, 0, "F12"});

    keys.push_back({0x0037, 2, "PrintScreen"});
    keys.push_back({0x0046, 0, "ScrollLock"});
  //keys.push_back({0x001d, 4, "Pause"});
    keys.push_back({0x0029, 0, "Tilde"});

    keys.push_back({0x0002, 0, "Num1"});
    keys.push_back({0x0003, 0, "Num2"});
    keys.push_back({0x0004, 0, "Num3"});
    keys.push_back({0x0005, 0, "Num4"});
    keys.push_back({0x0006, 0, "Num5"});
    keys.push_back({0x0007, 0, "Num6"});
    keys.push_back({0x0008, 0, "Num7"});
    keys.push_back({0x0009, 0, "Num8"});
    keys.push_back({0x000a, 0, "Num9"});
    keys.push_back({0x000b, 0, "Num0"});

    keys.push_back({0x000c, 0, "Dash"});
    keys.push_back({0x000d, 0, "Equal"});
    keys.push_back({0x000e, 0, "Backspace"});

    keys.push_back({0x0052, 2, "Insert"});
    keys.push_back({0x0053, 2, "Delete"});
    keys.push_back({0x0047, 2, "Home"});
    keys.push_back({0x004f, 2, "End"});
    keys.push_back({0x0049, 2, "PageUp"});
    keys.push_back({0x0051, 2, "PageDown"});

    keys.push_back({0x001e, 0, "A"});
    keys.push_back({0x0030, 0, "B"});
    keys.push_back({0x002e, 0, "C"});
    keys.push_back({0x0020, 0, "D"});
    keys.push_back({0x0012, 0, "E"});
    keys.push_back({0x0021, 0, "F"});
    keys.push_back({0x0022, 0, "G"});
    keys.push_back({0x0023, 0, "H"});
    keys.push_back({0x0017, 0, "I"});
    keys.push_back({0x0024, 0, "J"});
    keys.push_back({0x0025, 0, "K"});
    keys.push_back({0x0026, 0, "L"});
    keys.push_back({0x0032, 0, "M"});
    keys.push_back({0x0031, 0, "N"});
    keys.push_back({0x0018, 0, "O"});
    keys.push_back({0x0019, 0, "P"});
    keys.push_back({0x0010, 0, "Q"});
    keys.push_back({0x0013, 0, "R"});
    keys.push_back({0x001f, 0, "S"});
    keys.push_back({0x0014, 0, "T"});
    keys.push_back({0x0016, 0, "U"});
    keys.push_back({0x002f, 0, "V"});
    keys.push_back({0x0011, 0, "W"});
    keys.push_back({0x002d, 0, "X"});
    keys.push_back({0x0015, 0, "Y"});
    keys.push_back({0x002c, 0, "Z"});

    keys.push_back({0x001a, 0, "LeftBracket"});
    keys.push_back({0x001b, 0, "RightBracket"});
    keys.push_back({0x002b, 0, "Backslash"});
    keys.push_back({0x0027, 0, "Semicolon"});
    keys.push_back({0x0028, 0, "Apostrophe"});
    keys.push_back({0x0033, 0, "Comma"});
    keys.push_back({0x0034, 0, "Period"});
    keys.push_back({0x0035, 0, "Slash"});

    keys.push_back({0x004f, 0, "Keypad1"});
    keys.push_back({0x0050, 0, "Keypad2"});
    keys.push_back({0x0051, 0, "Keypad3"});
    keys.push_back({0x004b, 0, "Keypad4"});
    keys.push_back({0x004c, 0, "Keypad5"});
    keys.push_back({0x004d, 0, "Keypad6"});
    keys.push_back({0x0047, 0, "Keypad7"});
    keys.push_back({0x0048, 0, "Keypad8"});
    keys.push_back({0x0049, 0, "Keypad9"});
    keys.push_back({0x0052, 0, "Keypad0"});

    keys.push_back({0x0053, 0, "Point"});
    keys.push_back({0x001c, 2, "Enter"});
    keys.push_back({0x004e, 0, "Add"});
    keys.push_back({0x004a, 0, "Subtract"});
    keys.push_back({0x0037, 0, "Multiply"});
    keys.push_back({0x0035, 2, "Divide"});

  //keys.push_back({0x0045, 0, "NumLock"});
    keys.push_back({0x003a, 0, "CapsLock"});

    keys.push_back({0x0048, 2, "Up"});
    keys.push_back({0x0050, 2, "Down"});
    keys.push_back({0x004b, 2, "Left"});
    keys.push_back({0x004d, 2, "Right"});

    keys.push_back({0x000f, 0, "Tab"});
    keys.push_back({0x001c, 0, "Return"});
    keys.push_back({0x0039, 0, "Spacebar"});

    keys.push_back({0x002a, 0, "LeftShift"});
    keys.push_back({0x0036, 0, "RightShift"});
    keys.push_back({0x001d, 0, "LeftControl"});
    keys.push_back({0x001d, 2, "RightControl"});
    keys.push_back({0x0038, 0, "LeftAlt"});
    keys.push_back({0x0038, 2, "RightAlt"});
    keys.push_back({0x005b, 2, "LeftSuper"});
    keys.push_back({0x005c, 2, "RightSuper"});
    keys.push_back({0x005d, 2, "Menu"});

    kb.hid->setVendorID(HID::Keyboard::GenericVendorID);
    kb.hid->setProductID(HID::Keyboard::GenericProductID);
    kb.hid->setPathID(0);
    for(auto& key : keys) kb.hid->buttons().append(key.name);

    return true;
  }

  auto terminate() -> void {
    rawinput.updateKeyboard.reset();
  }
};
