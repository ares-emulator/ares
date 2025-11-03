struct Famicom : Emulator {
  Famicom();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
  auto loadTape(ares::Node::Object node, string location) -> bool override;
  auto unloadTape(ares::Node::Object node) -> void override;

  std::shared_ptr<mia::Pak> famicomDataRecorder{};
};

Famicom::Famicom() {
  manufacturer = "Nintendo";
  name = "Famicom";

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",         virtualPorts[id].pad.up);
    device.digital("Down",       virtualPorts[id].pad.down);
    device.digital("Left",       virtualPorts[id].pad.left);
    device.digital("Right",      virtualPorts[id].pad.right);
    device.digital("B",          virtualPorts[id].pad.west);
    device.digital("A",          virtualPorts[id].pad.south);
    device.digital("Select",     virtualPorts[id].pad.select);
    device.digital("Start",      virtualPorts[id].pad.start);
    device.digital("Microphone", virtualPorts[id].pad.north);
    port.append(device); }

  { InputDevice device{"Zapper"};
    device.relative("X",         virtualPorts[id].mouse.x);
    device.relative("Y",         virtualPorts[id].mouse.y);
    device.digital ("Trigger",   virtualPorts[id].mouse.left);
    port.append(device);
    }

    ports.push_back(port);
  }
}

auto Famicom::load() -> LoadResult {
  game = mia::Medium::create("Famicom");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("Famicom");
  result = system->load();
  if(result != successful) return result;

  auto region = Emulator::region();
  if(!ares::Famicom::load(root, {"[Nintendo] Famicom (", region, ")"})) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Gamepad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Gamepad");
    port->connect();
  }

  if(game->pak->attribute("system") == "EPSM") {
    if(auto port = root->find<ares::Node::Port>("Expansion Port")) {
      port->allocate("EPSM");
      port->connect();
    }
  }

  return successful;
}

auto Famicom::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto Famicom::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Famicom") return system->pak;
  if(node->name() == "Famicom Cartridge") return game->pak;
  if(node->name() == "Famicom Data Recorder") return famicomDataRecorder->pak;
  return {};
}

auto Famicom::input(ares::Node::Input::Input input) -> void {
  auto device = ares::Node::parent(input);
  if (!device) return;

  auto port = ares::Node::parent(device);
  if (!port) return;

  if (device->name() != "Family Keyboard") return Emulator::input(input);

  if (!program.keyboardCaptured) return;
  auto button = input->cast<ares::Node::Input::Button>();

  if (input->name() == "F1")          return button->setValue(inputKeyboard("F1"));
  if (input->name() == "F2")          return button->setValue(inputKeyboard("F2"));
  if (input->name() == "F3")          return button->setValue(inputKeyboard("F3"));
  if (input->name() == "F4")          return button->setValue(inputKeyboard("F4"));
  if (input->name() == "F5")          return button->setValue(inputKeyboard("F5"));
  if (input->name() == "F6")          return button->setValue(inputKeyboard("F6"));
  if (input->name() == "F7")          return button->setValue(inputKeyboard("F7"));
  if (input->name() == "F8")          return button->setValue(inputKeyboard("F8"));

  if (input->name() == "1")           return button->setValue(inputKeyboard("Num1"));
  if (input->name() == "2")           return button->setValue(inputKeyboard("Num2"));
  if (input->name() == "3")           return button->setValue(inputKeyboard("Num3"));
  if (input->name() == "4")           return button->setValue(inputKeyboard("Num4"));
  if (input->name() == "5")           return button->setValue(inputKeyboard("Num5"));
  if (input->name() == "6")           return button->setValue(inputKeyboard("Num6"));
  if (input->name() == "7")           return button->setValue(inputKeyboard("Num7"));
  if (input->name() == "8")           return button->setValue(inputKeyboard("Num8"));
  if (input->name() == "9")           return button->setValue(inputKeyboard("Num9"));
  if (input->name() == "0")           return button->setValue(inputKeyboard("Num0"));
  if (input->name() == "Minus")       return button->setValue(inputKeyboard("Dash"));
  if (input->name() == "^")           return button->setValue(inputKeyboard("Tilde"));
  if (input->name() == "Yen")         return button->setValue(inputKeyboard("Backslash"));
  if (input->name() == "Stop")        return button->setValue(inputKeyboard("Delete"));

  if (input->name() == "Escape")      return button->setValue(inputKeyboard("Escape"));
  if (input->name() == "Q")           return button->setValue(inputKeyboard("Q"));
  if (input->name() == "W")           return button->setValue(inputKeyboard("W"));
  if (input->name() == "E")           return button->setValue(inputKeyboard("E"));
  if (input->name() == "R")           return button->setValue(inputKeyboard("R"));
  if (input->name() == "T")           return button->setValue(inputKeyboard("T"));
  if (input->name() == "Y")           return button->setValue(inputKeyboard("Y"));
  if (input->name() == "U")           return button->setValue(inputKeyboard("U"));
  if (input->name() == "I")           return button->setValue(inputKeyboard("I"));
  if (input->name() == "O")           return button->setValue(inputKeyboard("O"));
  if (input->name() == "P")           return button->setValue(inputKeyboard("P"));
  if (input->name() == "@")           return button->setValue(inputKeyboard("F9"));
  if (input->name() == "[")           return button->setValue(inputKeyboard("LeftBracket"));
  if (input->name() == "Return")      return button->setValue(inputKeyboard("Return"));

  if (input->name() == "Control")     return button->setValue(inputKeyboard("LeftControl"));
  if (input->name() == "A")           return button->setValue(inputKeyboard("A"));
  if (input->name() == "S")           return button->setValue(inputKeyboard("S"));
  if (input->name() == "D")           return button->setValue(inputKeyboard("D"));
  if (input->name() == "F")           return button->setValue(inputKeyboard("F"));
  if (input->name() == "G")           return button->setValue(inputKeyboard("G"));
  if (input->name() == "H")           return button->setValue(inputKeyboard("H"));
  if (input->name() == "J")           return button->setValue(inputKeyboard("J"));
  if (input->name() == "K")           return button->setValue(inputKeyboard("K"));
  if (input->name() == "L")           return button->setValue(inputKeyboard("L"));
  if (input->name() == ";")           return button->setValue(inputKeyboard("Semicolon"));
  if (input->name() == ":")           return button->setValue(inputKeyboard("Apostrophe"));
  if (input->name() == "]")           return button->setValue(inputKeyboard("RightBracket"));
  if (input->name() == "Kana")        return button->setValue(inputKeyboard("End"));

  if (input->name() == "Left Shift")  return button->setValue(inputKeyboard("LeftShift"));
  if (input->name() == "Z")           return button->setValue(inputKeyboard("Z"));
  if (input->name() == "X")           return button->setValue(inputKeyboard("X"));
  if (input->name() == "C")           return button->setValue(inputKeyboard("C"));
  if (input->name() == "V")           return button->setValue(inputKeyboard("V"));
  if (input->name() == "B")           return button->setValue(inputKeyboard("B"));
  if (input->name() == "N")           return button->setValue(inputKeyboard("N"));
  if (input->name() == "M")           return button->setValue(inputKeyboard("M"));
  if (input->name() == ",")           return button->setValue(inputKeyboard("Comma"));
  if (input->name() == ".")           return button->setValue(inputKeyboard("Period"));
  if (input->name() == "/")           return button->setValue(inputKeyboard("Slash"));
  if (input->name() == "_")           return button->setValue(inputKeyboard("Equal"));
  if (input->name() == "Right Shift") return button->setValue(inputKeyboard("RightShift"));

  if (input->name() == "Graph")       return button->setValue(inputKeyboard("LeftAlt"));
  if (input->name() == "Spacebar")    return button->setValue(inputKeyboard("Space"));

  if (input->name() == "Home")        return button->setValue(inputKeyboard("Home"));
  if (input->name() == "Insert")      return button->setValue(inputKeyboard("Insert"));
  if (input->name() == "Delete")      return button->setValue(inputKeyboard("Backspace"));

  if (input->name() == "Up")          return button->setValue(inputKeyboard("Up"));
  if (input->name() == "Down")        return button->setValue(inputKeyboard("Down"));
  if (input->name() == "Left")        return button->setValue(inputKeyboard("Left"));
  if (input->name() == "Right")       return button->setValue(inputKeyboard("Right"));
}

auto Famicom::loadTape(ares::Node::Object node, string location) -> bool {
  if (node->name() == "Famicom Data Recorder") {
    famicomDataRecorder = mia::Medium::create("Tape");
    if (!location) {
      location = Emulator::load(famicomDataRecorder, settings.paths.home);
      if (!location) return false;
    }
    LoadResult result = famicomDataRecorder->load(location);
    if (result != successful) {
      famicomDataRecorder.reset();
      return false;
    }

    return true;
  }

  return false;
}

auto Famicom::unloadTape(ares::Node::Object node) -> void {
  if (node->name() == "Famicom Data Recorder") {
    famicomDataRecorder->save(famicomDataRecorder->location);
    famicomDataRecorder.reset();
  }
}
