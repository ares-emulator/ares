#pragma once

struct InputKeyboardXlib {
  Input& input;
  InputKeyboardXlib(Input& input) : input(input) {}

  shared_pointer<HID::Keyboard> hid{new HID::Keyboard};

  Display* display = nullptr;

  struct Key {
    string name;
    u32 keysym = 0;
    u32 keycode = 0;
  };
  vector<Key> keys;

  auto assign(u32 inputID, bool value) -> void {
    auto& group = hid->buttons();
    if(group.input(inputID).value() == value) return;
    input.doChange(hid, HID::Keyboard::GroupID::Button, inputID, group.input(inputID).value(), value);
    group.input(inputID).setValue(value);
  }

  auto poll(std::vector<shared_pointer<HID::Device>>& devices) -> void {
    char state[32];
    XQueryKeymap(display, state);

    u32 inputID = 0;
    for(auto& key : keys) {
      bool value = state[key.keycode >> 3] & (1 << (key.keycode & 7));
      assign(inputID++, value);
    }

    devices.push_back(hid);
  }

  auto initialize() -> bool {
    display = XOpenDisplay(0);

    keys.push_back({"Escape", XK_Escape});

    keys.push_back({"F1", XK_F1});
    keys.push_back({"F2", XK_F2});
    keys.push_back({"F3", XK_F3});
    keys.push_back({"F4", XK_F4});
    keys.push_back({"F5", XK_F5});
    keys.push_back({"F6", XK_F6});
    keys.push_back({"F7", XK_F7});
    keys.push_back({"F8", XK_F8});
    keys.push_back({"F9", XK_F9});
    keys.push_back({"F10", XK_F10});
    keys.push_back({"F11", XK_F11});
    keys.push_back({"F12", XK_F12});

    keys.push_back({"ScrollLock", XK_Scroll_Lock});
    keys.push_back({"Pause", XK_Pause});

    keys.push_back({"Tilde", XK_asciitilde});

    keys.push_back({"Num0", XK_0});
    keys.append({"Num1", XK_1});
    keys.append({"Num2", XK_2});
    keys.append({"Num3", XK_3});
    keys.append({"Num4", XK_4});
    keys.append({"Num5", XK_5});
    keys.append({"Num6", XK_6});
    keys.append({"Num7", XK_7});
    keys.append({"Num8", XK_8});
    keys.append({"Num9", XK_9});

    keys.append({"Dash", XK_minus});
    keys.append({"Equal", XK_equal});
    keys.append({"Backspace", XK_BackSpace});

    keys.push_back({"Insert", XK_Insert});
    keys.push_back({"Delete", XK_Delete});
    keys.push_back({"Home", XK_Home});
    keys.push_back({"End", XK_End});
    keys.push_back({"PageUp", XK_Prior});
    keys.push_back({"PageDown", XK_Next});

    keys.push_back({"A", XK_A});
    keys.push_back({"B", XK_B});
    keys.push_back({"C", XK_C});
    keys.push_back({"D", XK_D});
    keys.push_back({"E", XK_E});
    keys.push_back({"F", XK_F});
    keys.push_back({"G", XK_G});
    keys.push_back({"H", XK_H});
    keys.push_back({"I", XK_I});
    keys.push_back({"J", XK_J});
    keys.push_back({"K", XK_K});
    keys.push_back({"L", XK_L});
    keys.push_back({"M", XK_M});
    keys.push_back({"N", XK_N});
    keys.push_back({"O", XK_O});
    keys.push_back({"P", XK_P});
    keys.push_back({"Q", XK_Q});
    keys.push_back({"R", XK_R});
    keys.push_back({"S", XK_S});
    keys.push_back({"T", XK_T});
    keys.push_back({"U", XK_U});
    keys.push_back({"V", XK_V});
    keys.push_back({"W", XK_W});
    keys.push_back({"X", XK_X});
    keys.push_back({"Y", XK_Y});
    keys.push_back({"Z", XK_Z});

    keys.push_back({"LeftBracket", XK_bracketleft});
    keys.push_back({"RightBracket", XK_bracketright});
    keys.push_back({"Backslash", XK_backslash});
    keys.push_back({"Semicolon", XK_semicolon});
    keys.push_back({"Apostrophe", XK_apostrophe});
    keys.push_back({"Comma", XK_comma});
    keys.push_back({"Period", XK_period});
    keys.push_back({"Slash", XK_slash});

    keys.push_back({"Keypad0", XK_KP_0});
    keys.push_back({"Keypad1", XK_KP_1});
    keys.push_back({"Keypad2", XK_KP_2});
    keys.push_back({"Keypad3", XK_KP_3});
    keys.push_back({"Keypad4", XK_KP_4});
    keys.push_back({"Keypad5", XK_KP_5});
    keys.push_back({"Keypad6", XK_KP_6});
    keys.push_back({"Keypad7", XK_KP_7});
    keys.push_back({"Keypad8", XK_KP_8});
    keys.push_back({"Keypad9", XK_KP_9});

    keys.push_back({"Add", XK_KP_Add});
    keys.push_back({"Subtract", XK_KP_Subtract});
    keys.push_back({"Multiply", XK_KP_Multiply});
    keys.push_back({"Divide", XK_KP_Divide});
    keys.push_back({"Enter", XK_KP_Enter});

    keys.push_back({"Up", XK_Up});
    keys.push_back({"Down", XK_Down});
    keys.push_back({"Left", XK_Left});
    keys.push_back({"Right", XK_Right});

    keys.push_back({"Tab", XK_Tab});
    keys.push_back({"Return", XK_Return});
    keys.push_back({"Spacebar", XK_space});

    keys.push_back({"LeftControl", XK_Control_L});
    keys.push_back({"RightControl", XK_Control_R});
    keys.push_back({"LeftAlt", XK_Alt_L});
    keys.push_back({"RightAlt", XK_Alt_R});
    keys.push_back({"LeftShift", XK_Shift_L});
    keys.push_back({"RightShift", XK_Shift_R});
    keys.push_back({"LeftSuper", XK_Super_L});
    keys.push_back({"RightSuper", XK_Super_R});
    keys.push_back({"Menu", XK_Menu});

    hid->setVendorID(HID::Keyboard::GenericVendorID);
    hid->setProductID(HID::Keyboard::GenericProductID);
    hid->setPathID(0);

    for(auto& key : keys) {
      hid->buttons().append(key.name);
      key.keycode = XKeysymToKeycode(display, key.keysym);
    }

    return true;
  }

  auto terminate() -> void {
    if(display) {
      XCloseDisplay(display);
      display = nullptr;
    }
  }
};
