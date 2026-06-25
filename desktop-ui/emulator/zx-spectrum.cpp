struct ZXSpectrum : Emulator {
  ZXSpectrum();
  auto load() -> LoadResult override;
  auto load(Menu) -> void override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
  auto loadTape(ares::Node::Object node, string location) -> bool override;
  auto unloadTape(ares::Node::Object node) -> void override;

  std::shared_ptr<mia::Pak> tape;
};

ZXSpectrum::ZXSpectrum() {
  manufacturer = "Sinclair";
  name = "ZX Spectrum";
}

auto ZXSpectrum::load() -> LoadResult {
  game = mia::Medium::create("ZX Spectrum");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("ZX Spectrum");
  result = system->load();
  if(result != successful) return result;

  if(!ares::ZXSpectrum::load(root, "[Sinclair] ZX Spectrum")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Tape Deck/Tray")) {
    tape = game;
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Keyboard")) {
    port->allocate("Original");
    port->connect();
  }

  return successful;
}

auto ZXSpectrum::load(Menu menu) -> void {
  if(auto tape = root->find<ares::Node::Tape>("ZX Spectrum Tape")) {
    MenuCheckItem playingItem{&menu};
    playingItem.setText("Play Tape").setChecked(tape->playing()).onToggle([=, this] {
      if(auto tape = root->find<ares::Node::Tape>("ZX Spectrum Tape")) {
        if(playingItem.checked()) tape->play();
        else tape->stop();
      };
    });
  }
}


auto ZXSpectrum::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  if(tape && tape != game) tape->save(tape->location);
  return true;
}

auto ZXSpectrum::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "ZX Spectrum") return system->pak;
  if(node->name() == "ZX Spectrum Tape") return tape ? tape->pak : game->pak;
  return {};
}

auto ZXSpectrum::loadTape(ares::Node::Object node, string location) -> bool {
  if(node->name() == "ZX Spectrum Tape") {
    tape = mia::Medium::create("ZX Spectrum");
    if(!location) {
      location = Emulator::load(tape, settings.paths.home);
      if(!location) return false;
    }
    LoadResult result = tape->load(location);
    if(result != successful) {
      tape.reset();
      return false;
    }

    return true;
  }

  return false;
}

auto ZXSpectrum::unloadTape(ares::Node::Object node) -> void {
  if(node->name() == "ZX Spectrum Tape") {
    if(tape) tape->save(tape->location);
    tape.reset();
  }
}

auto ZXSpectrum::input(ares::Node::Input::Input input) -> void {
  auto device = ares::Node::parent(input);
  if(!device) return;

  auto port = ares::Node::parent(device);
  if(!port) return;

  if (!program.keyboardCaptured) return;
  auto button = input->cast<ares::Node::Input::Button>();

  // Host Cursor Keys simulate ZX Spectrum cursor actions
  if(inputKeyboard("Up"))
    if(input->name() == "CAPS SHIFT" || input->name() == "7") return button->setValue(true);
  if(inputKeyboard("Down"))
    if(input->name() == "CAPS SHIFT" || input->name() == "6") return button->setValue(true);
  if(inputKeyboard("Left"))
    if(input->name() == "CAPS SHIFT" || input->name() == "5") return button->setValue(true);
  if(inputKeyboard("Right"))
    if(input->name() == "CAPS SHIFT" || input->name() == "8") return button->setValue(true);

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
  if (input->name() == "CAPS SHIFT")   return button->setValue(inputKeyboard("LeftShift"));
  if (input->name() == "SYMBOL SHIFT") return button->setValue(inputKeyboard("LeftControl"));
  if (input->name() == "ENTER")        return button->setValue(inputKeyboard("Return"));
  if (input->name() == "SPACE BREAK")  return button->setValue(inputKeyboard("Spacebar"));
}
