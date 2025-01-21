struct MSX : Emulator {
  MSX();
  auto load() -> LoadResult override;
  auto load(Menu) -> void override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

MSX::MSX() {
  manufacturer = "Microsoft";
  name = "MSX";

  // TODO: Support other region bios versions
  firmware.append({"BIOS", "Japan", "413a2b601a94b3792e054be2439cc77a1819cceadbfa9542f88d51c7480f2ef0"});

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",    virtualPorts[id].pad.up);
    device.digital("Down",  virtualPorts[id].pad.down);
    device.digital("Left",  virtualPorts[id].pad.left);
    device.digital("Right", virtualPorts[id].pad.right);
    device.digital("A",     virtualPorts[id].pad.east);
    device.digital("B",     virtualPorts[id].pad.south);
    port.append(device); }

  { InputDevice device{"Arkanoid Vaus Paddle"};
    device.analog ("L-Left",  virtualPorts[id].pad.lstick_left);
    device.analog ("L-Right", virtualPorts[id].pad.lstick_right);
    device.analog ("X-Axis",  virtualPorts[id].pad.lstick_left, virtualPorts[id].pad.lstick_right);
    device.digital("Button",  virtualPorts[id].pad.south);
    port.append(device); }

    ports.append(port);
  }
}

auto MSX::load() -> LoadResult {
  game = mia::Medium::create("MSX");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;
  bool isTape = game->pak->attribute("tape").boolean();

  system = mia::System::create("MSX");
  result = system->load(firmware[0].location);
  if(result != successful) {
    result.firmwareSystemName = "MSX";
    result.firmwareType = firmware[0].type;
    result.firmwareRegion = firmware[0].region;
    result.result = noFirmware;
    return result;
  }

  auto region = Emulator::region();
  if(!ares::MSX::load(root, {"[Microsoft] MSX (", region, ")"})) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    if(!isTape) port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Tape Deck/Tray")) {
    port->allocate();
    if(isTape) port->connect();
  }

  auto device = "Gamepad";
  if(game->pak->attribute("vauspaddle").boolean()) device = "Arkanoid Vaus Paddle";

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate(device);
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate(device);
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Keyboard")) {
    port->allocate("Japanese");
    port->connect();
  }

  return successful;
}

auto MSX::load(Menu menu) -> void {
  if(auto playing = root->find<ares::Node::Setting::Boolean>("Tape Deck/Playing")) {
    MenuCheckItem playingItem{&menu};
    playingItem.setText("Play Tape").setChecked(playing->value()).onToggle([=] {
      if(auto playing = root->find<ares::Node::Setting::Boolean>("Tape Deck/Playing")) {
        playing->setValue(playingItem.checked());
      }
    });
  }
}


auto MSX::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto MSX::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "MSX") return system->pak;
  if(node->name() == "MSX Cartridge") return game->pak;
  if(node->name() == "MSX Tape") return game->pak;
  return {};
}

auto MSX::input(ares::Node::Input::Input input) -> void {
  auto device = ares::Node::parent(input);
  if(!device) return;

  auto port = ares::Node::parent(device);
  if(!port) return;

  if (port->name() != "Keyboard") return Emulator::input(input);

  if (!program.keyboardCaptured) return;
  auto button = input->cast<ares::Node::Input::Button>();

  if (input->name() == "0 わ を")   return button->setValue(inputKeyboard("Num0"));
  if (input->name() == "1 ! ぬ")    return button->setValue(inputKeyboard("Num1"));
  if (input->name() == "2 \" ふ")   return button->setValue(inputKeyboard("Num2"));
  if (input->name() == "3 # あ ぁ") return button->setValue(inputKeyboard("Num3"));
  if (input->name() == "4 $ う ぅ") return button->setValue(inputKeyboard("Num4"));
  if (input->name() == "5 % え ぇ") return button->setValue(inputKeyboard("Num5"));
  if (input->name() == "6 & お ぉ") return button->setValue(inputKeyboard("Num6"));
  if (input->name() == "7 ’ や ゃ") return button->setValue(inputKeyboard("Num7"));
  if (input->name() == "8 ( ゆ ゅ") return button->setValue(inputKeyboard("Num8"));
  if (input->name() == "9 ) よ ょ") return button->setValue(inputKeyboard("Num9"));
  if (input->name() == "- = ほ")    return button->setValue(inputKeyboard("Dash"));
  if (input->name() == "^ ~ へ")    return button->setValue(inputKeyboard("Tilde"));
  if (input->name() == "¥ | ー")    return button->setValue(inputKeyboard("Equal"));
  if (input->name() == "@ ‘ \"")    return button->setValue(inputKeyboard("Apostrophe"));
  if (input->name() == "[ { 。")    return button->setValue(inputKeyboard("LeftBracket"));
  if (input->name() == "; + れ")    return button->setValue(inputKeyboard("Semicolon"));
  if (input->name() == ": * け")    return button->setValue(inputKeyboard("Backslash"));
  if (input->name() == "] } む")    return button->setValue(inputKeyboard("RightBracket"));
  if (input->name() == ", < ね `")  return button->setValue(inputKeyboard("Comma"));
  if (input->name() == ". > る 。") return button->setValue(inputKeyboard("Period"));
  if (input->name() == "/ ? め .")  return button->setValue(inputKeyboard("Slash"));
  if (input->name() == "- ろ")      return button->setValue(inputKeyboard("Dash"));
  if (input->name() == "A ち")      return button->setValue(inputKeyboard("A"));
  if (input->name() == "B こ")      return button->setValue(inputKeyboard("B"));
  if (input->name() == "C そ")      return button->setValue(inputKeyboard("C"));
  if (input->name() == "D し")      return button->setValue(inputKeyboard("D"));
  if (input->name() == "E い ぃ")   return button->setValue(inputKeyboard("E"));
  if (input->name() == "F は")      return button->setValue(inputKeyboard("F"));
  if (input->name() == "G き")      return button->setValue(inputKeyboard("G"));
  if (input->name() == "H く")      return button->setValue(inputKeyboard("H"));
  if (input->name() == "I に")      return button->setValue(inputKeyboard("I"));
  if (input->name() == "J ま")      return button->setValue(inputKeyboard("J"));
  if (input->name() == "K の")      return button->setValue(inputKeyboard("K"));
  if (input->name() == "L り")      return button->setValue(inputKeyboard("L"));
  if (input->name() == "M も")      return button->setValue(inputKeyboard("M"));
  if (input->name() == "N み")      return button->setValue(inputKeyboard("N"));
  if (input->name() == "O ら")      return button->setValue(inputKeyboard("O"));
  if (input->name() == "P せ")      return button->setValue(inputKeyboard("P"));
  if (input->name() == "Q た")      return button->setValue(inputKeyboard("Q"));
  if (input->name() == "R す")      return button->setValue(inputKeyboard("R"));
  if (input->name() == "S と")      return button->setValue(inputKeyboard("S"));
  if (input->name() == "T か")      return button->setValue(inputKeyboard("T"));
  if (input->name() == "U な")      return button->setValue(inputKeyboard("U"));
  if (input->name() == "V ひ")      return button->setValue(inputKeyboard("V"));
  if (input->name() == "W て")      return button->setValue(inputKeyboard("W"));
  if (input->name() == "X さ")      return button->setValue(inputKeyboard("X"));
  if (input->name() == "Y ん")      return button->setValue(inputKeyboard("Y"));
  if (input->name() == "Z つ っ")   return button->setValue(inputKeyboard("Z"));
  if (input->name() == "SHIFT")     return button->setValue(inputKeyboard("LeftShift"));
  if (input->name() == "CTRL")      return button->setValue(inputKeyboard("LeftControl"));
  if (input->name() == "GRAPH")     return button->setValue(inputKeyboard("LeftAlt"));
  if (input->name() == "CAPS")      return button->setValue(inputKeyboard("RightShift"));
  if (input->name() == "かな")      return button->setValue(inputKeyboard("RightAlt"));
  if (input->name() == "F1 F6")     return button->setValue(inputKeyboard("F1"));
  if (input->name() == "F2 F7")     return button->setValue(inputKeyboard("F2"));
  if (input->name() == "F3 F8" )    return button->setValue(inputKeyboard("F3"));
  if (input->name() == "F4 F9")     return button->setValue(inputKeyboard("F4"));
  if (input->name() == "F5 F10")    return button->setValue(inputKeyboard("F5"));
  if (input->name() == "ESC")       return button->setValue(inputKeyboard("Escape"));
  if (input->name() == "TAB")       return button->setValue(inputKeyboard("Tab"));
  if (input->name() == "STOP")      return button->setValue(inputKeyboard("PageUp"));
  if (input->name() == "BS")        return button->setValue(inputKeyboard("Backspace"));
  if (input->name() == "SELECT")    return button->setValue(inputKeyboard("End"));
  if (input->name() == "RETURN")    return button->setValue(inputKeyboard("Return"));
  if (input->name() == "SPACE")     return button->setValue(inputKeyboard("Spacebar"));
  if (input->name() == "CLS/HOME")  return button->setValue(inputKeyboard("Home"));
  if (input->name() == "INS")       return button->setValue(inputKeyboard("Insert"));
  if (input->name() == "DEL")       return button->setValue(inputKeyboard("Delete"));
  if (input->name() == "←")         return button->setValue(inputKeyboard("Left"));
  if (input->name() == "↑")         return button->setValue(inputKeyboard("Up"));
  if (input->name() == "↓")         return button->setValue(inputKeyboard("Down"));
  if (input->name() == "→")         return button->setValue(inputKeyboard("Right"));
  if (input->name() == "*")         return button->setValue(inputKeyboard("Multiply"));
  if (input->name() == "+")         return button->setValue(inputKeyboard("Add"));
  if (input->name() == "/")         return button->setValue(inputKeyboard("Divide"));
  if (input->name() == "0")         return button->setValue(inputKeyboard("Keypad0"));
  if (input->name() == "1")         return button->setValue(inputKeyboard("Keypad1"));
  if (input->name() == "2")         return button->setValue(inputKeyboard("Keypad2"));
  if (input->name() == "3")         return button->setValue(inputKeyboard("Keypad3"));
  if (input->name() == "4")         return button->setValue(inputKeyboard("Keypad4"));
  if (input->name() == "5")         return button->setValue(inputKeyboard("Keypad5"));
  if (input->name() == "6")         return button->setValue(inputKeyboard("Keypad6"));
  if (input->name() == "7")         return button->setValue(inputKeyboard("Keypad7"));
  if (input->name() == "8")         return button->setValue(inputKeyboard("Keypad8"));
  if (input->name() == "9")         return button->setValue(inputKeyboard("Keypad*"));
  if (input->name() == "-")         return button->setValue(inputKeyboard("Subtract"));
  if (input->name() == ",")         return button->setValue(inputKeyboard("Pagedown"));
  if (input->name() == ".")         return button->setValue(inputKeyboard("Point"));
  if (input->name() == "実行")      return button->setValue(inputKeyboard("LeftSuper"));
  if (input->name() == "取消")      return button->setValue(inputKeyboard("RightSuper"));
}
