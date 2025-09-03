struct SC3000 : Emulator {
  SC3000();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

SC3000::SC3000() {
  manufacturer = "Sega";
  name = "SC-3000";

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",    virtualPorts[id].pad.up);
    device.digital("Down",  virtualPorts[id].pad.down);
    device.digital("Left",  virtualPorts[id].pad.left);
    device.digital("Right", virtualPorts[id].pad.right);
    device.digital("1",     virtualPorts[id].pad.south);
    device.digital("2",     virtualPorts[id].pad.east);
    port.append(device); }

    ports.push_back(port);
  }
}

auto SC3000::load() -> LoadResult {
  game = mia::Medium::create("SC-3000");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("SC-3000");
  result = system->load();
  if(result != successful) return result;

  auto region = Emulator::region();
  if(!ares::SG1000::load(root, {"[Sega] SC-3000 (", region, ")"})) return otherError;

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

  return successful;
}

auto SC3000::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto SC3000::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "SC-3000") return system->pak;
  if(node->name() == "SC-3000 Cartridge") return game->pak;
  return {};
}


auto SC3000::input(ares::Node::Input::Input input) -> void {
  auto device = ares::Node::parent(input);
  if(!device) return;

  auto port = ares::Node::parent(device);
  if(!port) return;

  if (!program.keyboardCaptured) return;
  auto button = input->cast<ares::Node::Input::Button>();

  if (input->name() == "0") return button->setValue(inputKeyboard("Num0"));
  if (input->name() == "1") return button->setValue(inputKeyboard("Num1"));
  if (input->name() == "2") return button->setValue(inputKeyboard("Num2"));
  if (input->name() == "3") return button->setValue(inputKeyboard("Num3"));
  if (input->name() == "4") return button->setValue(inputKeyboard("Num4"));
  if (input->name() == "5") return button->setValue(inputKeyboard("Num5"));
  if (input->name() == "6") return button->setValue(inputKeyboard("Num6"));
  if (input->name() == "7") return button->setValue(inputKeyboard("Num7"));
  if (input->name() == "8") return button->setValue(inputKeyboard("Num8"));
  if (input->name() == "9") return button->setValue(inputKeyboard("Num9"));
  if (input->name() == "A") return button->setValue(inputKeyboard("A"));
  if (input->name() == "B") return button->setValue(inputKeyboard("B"));
  if (input->name() == "C") return button->setValue(inputKeyboard("C"));
  if (input->name() == "D") return button->setValue(inputKeyboard("D"));
  if (input->name() == "E") return button->setValue(inputKeyboard("E"));
  if (input->name() == "F") return button->setValue(inputKeyboard("F"));
  if (input->name() == "G") return button->setValue(inputKeyboard("G"));
  if (input->name() == "H") return button->setValue(inputKeyboard("H"));
  if (input->name() == "I") return button->setValue(inputKeyboard("I"));
  if (input->name() == "J") return button->setValue(inputKeyboard("J"));
  if (input->name() == "K") return button->setValue(inputKeyboard("K"));
  if (input->name() == "L") return button->setValue(inputKeyboard("L"));
  if (input->name() == "M") return button->setValue(inputKeyboard("M"));
  if (input->name() == "N") return button->setValue(inputKeyboard("N"));
  if (input->name() == "O") return button->setValue(inputKeyboard("O"));
  if (input->name() == "P") return button->setValue(inputKeyboard("P"));
  if (input->name() == "Q") return button->setValue(inputKeyboard("Q"));
  if (input->name() == "R") return button->setValue(inputKeyboard("R"));
  if (input->name() == "S") return button->setValue(inputKeyboard("S"));
  if (input->name() == "T") return button->setValue(inputKeyboard("T"));
  if (input->name() == "U") return button->setValue(inputKeyboard("U"));
  if (input->name() == "V") return button->setValue(inputKeyboard("V"));
  if (input->name() == "W") return button->setValue(inputKeyboard("W"));
  if (input->name() == "X") return button->setValue(inputKeyboard("X"));
  if (input->name() == "Y") return button->setValue(inputKeyboard("Y"));
  if (input->name() == "Z") return button->setValue(inputKeyboard("Z"));

  if (input->name() == ",") return button->setValue(inputKeyboard("Comma"));
  if (input->name() == ".") return button->setValue(inputKeyboard("Period"));
  if (input->name() == "/") return button->setValue(inputKeyboard("Slash"));
  if (input->name() == ";") return button->setValue(inputKeyboard("Semicolon"));
  if (input->name() == "-") return button->setValue(inputKeyboard("Dash"));
  if (input->name() == ":") return button->setValue(inputKeyboard("Equal"));

  if (input->name() == "[") return button->setValue(inputKeyboard("LeftBracket"));
  if (input->name() == "]") return button->setValue(inputKeyboard("RightBracket"));
  if (input->name() == "@") return button->setValue(inputKeyboard("Apostrophe"));
  if (input->name() == "^") return button->setValue(inputKeyboard("RightControl"));

  if (input->name() == "UA") return button->setValue(inputKeyboard("Up"));
  if (input->name() == "DA") return button->setValue(inputKeyboard("Down"));
  if (input->name() == "LA") return button->setValue(inputKeyboard("Left"));
  if (input->name() == "RA") return button->setValue(inputKeyboard("Right"));
  if (input->name() == "HC") return button->setValue(inputKeyboard("Home"));
  if (input->name() == "ID") return button->setValue(inputKeyboard("Insert") || inputKeyboard("Backspace"));
  if (input->name() == "CR") return button->setValue(inputKeyboard("Return"));
  if (input->name() == "ED") return button->setValue(inputKeyboard("RightAlt"));
  if (input->name() == "PI") return button->setValue(inputKeyboard("RightShift"));

  if (input->name() == "YEN") return button->setValue(inputKeyboard("Backslash"));
  if (input->name() == "SPC") return button->setValue(inputKeyboard("Spacebar"));
  if (input->name() == "BRK") return button->setValue(inputKeyboard("Escape"));
  if (input->name() == "FNC") return button->setValue(inputKeyboard("Tab"));
  if (input->name() == "SHF") return button->setValue(inputKeyboard("LeftShift"));
  if (input->name() == "CTL") return button->setValue(inputKeyboard("LeftControl"));
  if (input->name() == "GRP") return button->setValue(inputKeyboard("LeftAlt"));

}
