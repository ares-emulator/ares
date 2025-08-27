#pragma once

struct InputKeyboardQuartz {
  Input& input;
  InputKeyboardQuartz(Input& input) : input(input) {}

  shared_pointer<HID::Keyboard> hid{new HID::Keyboard};

  struct Key {
    string name;
    u32 id = 0;
  };
  std::vector<Key> keys;

  auto assign(u32 inputID, bool value) -> void {
    auto& group = hid->buttons();
    if(group.input(inputID).value() == value) return;
    input.doChange(hid, HID::Keyboard::GroupID::Button, inputID, group.input(inputID).value(), value);
    group.input(inputID).setValue(value);
  }

  auto poll(std::vector<shared_pointer<HID::Device>>& devices) -> void {
    u32 inputID = 0;
    for(auto& key : keys) {
      bool value = CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, key.id);
      assign(inputID++, value);
    }
    devices.push_back(hid);
  }

  auto initialize() -> bool {
    keys.push_back({"Escape", kVK_Escape});
    keys.push_back({"F1", kVK_F1});
    keys.push_back({"F2", kVK_F2});
    keys.push_back({"F3", kVK_F3});
    keys.push_back({"F4", kVK_F4});
    keys.push_back({"F5", kVK_F5});
    keys.push_back({"F6", kVK_F6});
    keys.push_back({"F7", kVK_F7});
    keys.push_back({"F8", kVK_F8});
    keys.push_back({"F9", kVK_F9});
    keys.push_back({"F10", kVK_F10});
    keys.push_back({"F11", kVK_F11});
    keys.push_back({"F12", kVK_F12});
    keys.push_back({"F13", kVK_F13});
    keys.push_back({"F14", kVK_F14});
    keys.push_back({"F15", kVK_F15});
    keys.push_back({"F16", kVK_F16});
    keys.push_back({"F17", kVK_F17});
    keys.push_back({"F18", kVK_F18});
    keys.push_back({"F19", kVK_F19});
    keys.push_back({"F20", kVK_F20});

    keys.push_back({"Tilde", kVK_ANSI_Grave});
    keys.push_back({"Num1", kVK_ANSI_1});
    keys.push_back({"Num2", kVK_ANSI_2});
    keys.push_back({"Num3", kVK_ANSI_3});
    keys.push_back({"Num4", kVK_ANSI_4});
    keys.push_back({"Num5", kVK_ANSI_5});
    keys.push_back({"Num6", kVK_ANSI_6});
    keys.push_back({"Num7", kVK_ANSI_7});
    keys.push_back({"Num8", kVK_ANSI_8});
    keys.push_back({"Num9", kVK_ANSI_9});
    keys.push_back({"Num0", kVK_ANSI_0});

    keys.push_back({"Dash", kVK_ANSI_Minus});
    keys.push_back({"Equal", kVK_ANSI_Equal});
    keys.push_back({"Delete", kVK_Delete});

    keys.push_back({"Erase", kVK_ForwardDelete});
    keys.push_back({"Home", kVK_Home});
    keys.push_back({"End", kVK_End});
    keys.push_back({"PageUp", kVK_PageUp});
    keys.push_back({"PageDown", kVK_PageDown});

    keys.push_back({"A", kVK_ANSI_A});
    keys.push_back({"B", kVK_ANSI_B});
    keys.push_back({"C", kVK_ANSI_C});
    keys.push_back({"D", kVK_ANSI_D});
    keys.push_back({"E", kVK_ANSI_E});
    keys.push_back({"F", kVK_ANSI_F});
    keys.push_back({"G", kVK_ANSI_G});
    keys.push_back({"H", kVK_ANSI_H});
    keys.push_back({"I", kVK_ANSI_I});
    keys.push_back({"J", kVK_ANSI_J});
    keys.push_back({"K", kVK_ANSI_K});
    keys.push_back({"L", kVK_ANSI_L});
    keys.push_back({"M", kVK_ANSI_M});
    keys.push_back({"N", kVK_ANSI_N});
    keys.push_back({"O", kVK_ANSI_O});
    keys.push_back({"P", kVK_ANSI_P});
    keys.push_back({"Q", kVK_ANSI_Q});
    keys.push_back({"R", kVK_ANSI_R});
    keys.push_back({"S", kVK_ANSI_S});
    keys.push_back({"T", kVK_ANSI_T});
    keys.push_back({"U", kVK_ANSI_U});
    keys.push_back({"V", kVK_ANSI_V});
    keys.push_back({"W", kVK_ANSI_W});
    keys.push_back({"X", kVK_ANSI_X});
    keys.push_back({"Y", kVK_ANSI_Y});
    keys.push_back({"Z", kVK_ANSI_Z});

    keys.push_back({"LeftBracket", kVK_ANSI_LeftBracket});
    keys.push_back({"RightBracket", kVK_ANSI_RightBracket});
    keys.push_back({"Backslash", kVK_ANSI_Backslash});
    keys.push_back({"Semicolon", kVK_ANSI_Semicolon});
    keys.push_back({"Apostrophe", kVK_ANSI_Quote});
    keys.push_back({"Comma", kVK_ANSI_Comma});
    keys.push_back({"Period", kVK_ANSI_Period});
    keys.push_back({"Slash", kVK_ANSI_Slash});

    keys.push_back({"Keypad1", kVK_ANSI_Keypad1});
    keys.push_back({"Keypad2", kVK_ANSI_Keypad2});
    keys.push_back({"Keypad3", kVK_ANSI_Keypad3});
    keys.push_back({"Keypad4", kVK_ANSI_Keypad4});
    keys.push_back({"Keypad5", kVK_ANSI_Keypad5});
    keys.push_back({"Keypad6", kVK_ANSI_Keypad6});
    keys.push_back({"Keypad7", kVK_ANSI_Keypad7});
    keys.push_back({"Keypad8", kVK_ANSI_Keypad8});
    keys.push_back({"Keypad9", kVK_ANSI_Keypad9});
    keys.push_back({"Keypad0", kVK_ANSI_Keypad0});

    keys.push_back({"Clear", kVK_ANSI_KeypadClear});
    keys.push_back({"Equals", kVK_ANSI_KeypadEquals});
    keys.push_back({"Divide", kVK_ANSI_KeypadDivide});
    keys.push_back({"Multiply", kVK_ANSI_KeypadMultiply});
    keys.push_back({"Subtract", kVK_ANSI_KeypadMinus});
    keys.push_back({"Add", kVK_ANSI_KeypadPlus});
    keys.push_back({"Enter", kVK_ANSI_KeypadEnter});
    keys.push_back({"Decimal", kVK_ANSI_KeypadDecimal});

    keys.push_back({"Up", kVK_UpArrow});
    keys.push_back({"Down", kVK_DownArrow});
    keys.push_back({"Left", kVK_LeftArrow});
    keys.push_back({"Right", kVK_RightArrow});

    keys.push_back({"Tab", kVK_Tab});
    keys.push_back({"Return", kVK_Return});
    keys.push_back({"Spacebar", kVK_Space});
    keys.push_back({"Shift", kVK_Shift});
    keys.push_back({"Control", kVK_Control});
    keys.push_back({"Option", kVK_Option});
    keys.push_back({"Command", kVK_Command});

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
