struct Famicom : Emulator {
  Famicom();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto loadTape(ares::Node::Object node, string location) -> bool override;
  auto unloadTape(ares::Node::Object node) -> void override;

  shared_pointer<mia::Pak> familyBasicDataRecorder{};
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
    port.append(device); }

    ports.append(port);
  }

  {
    InputPort port{"Expansion Port"};
  { InputDevice device{"Family BASIC Keyboard"};
    device.digital("F1",          virtualPorts[2].keyboard.f1);
    device.digital("F2",          virtualPorts[2].keyboard.f2);
    device.digital("F3",          virtualPorts[2].keyboard.f3);
    device.digital("F4",          virtualPorts[2].keyboard.f4);
    device.digital("F5",          virtualPorts[2].keyboard.f5);
    device.digital("F6",          virtualPorts[2].keyboard.f6);
    device.digital("F7",          virtualPorts[2].keyboard.f7);
    device.digital("F8",          virtualPorts[2].keyboard.f8);

    device.digital("1",           virtualPorts[2].keyboard.num1);
    device.digital("2",           virtualPorts[2].keyboard.num2);
    device.digital("3",           virtualPorts[2].keyboard.num3);
    device.digital("4",           virtualPorts[2].keyboard.num4);
    device.digital("5",           virtualPorts[2].keyboard.num5);
    device.digital("6",           virtualPorts[2].keyboard.num6);
    device.digital("7",           virtualPorts[2].keyboard.num7);
    device.digital("8",           virtualPorts[2].keyboard.num8);
    device.digital("9",           virtualPorts[2].keyboard.num9);
    device.digital("0",           virtualPorts[2].keyboard.num0);
    device.digital("Minus",       virtualPorts[2].keyboard.dash);
    device.digital("^",           virtualPorts[2].keyboard.tilde);
    device.digital("Yen",         virtualPorts[2].keyboard.equals);
    device.digital("Stop",        virtualPorts[2].keyboard.backspace);

    device.digital("Escape",      virtualPorts[2].keyboard.esc);
    device.digital("Q",           virtualPorts[2].keyboard.q);
    device.digital("W",           virtualPorts[2].keyboard.w);
    device.digital("E",           virtualPorts[2].keyboard.e);
    device.digital("R",           virtualPorts[2].keyboard.r);
    device.digital("T",           virtualPorts[2].keyboard.t);
    device.digital("Y",           virtualPorts[2].keyboard.y);
    device.digital("U",           virtualPorts[2].keyboard.u);
    device.digital("I",           virtualPorts[2].keyboard.i);
    device.digital("O",           virtualPorts[2].keyboard.o);
    device.digital("P",           virtualPorts[2].keyboard.p);
    device.digital("@",           virtualPorts[2].keyboard.lbracket);
    device.digital("Return",      virtualPorts[2].keyboard.return_);

    device.digital("Control",     virtualPorts[2].keyboard.lctrl);
    device.digital("A",           virtualPorts[2].keyboard.a);
    device.digital("S",           virtualPorts[2].keyboard.s);
    device.digital("D",           virtualPorts[2].keyboard.d);
    device.digital("F",           virtualPorts[2].keyboard.f);
    device.digital("G",           virtualPorts[2].keyboard.g);
    device.digital("H",           virtualPorts[2].keyboard.h);
    device.digital("J",           virtualPorts[2].keyboard.j);
    device.digital("K",           virtualPorts[2].keyboard.k);
    device.digital("L",           virtualPorts[2].keyboard.l);
    device.digital(";",           virtualPorts[2].keyboard.semicolon);
    device.digital(":",           virtualPorts[2].keyboard.apostrophe);
    device.digital("]",           virtualPorts[2].keyboard.rbracket);
    device.digital("Kana",        virtualPorts[2].keyboard.rctrl);

    device.digital("Left Shift",  virtualPorts[2].keyboard.lshift);
    device.digital("Z",           virtualPorts[2].keyboard.z);
    device.digital("X",           virtualPorts[2].keyboard.x);
    device.digital("C",           virtualPorts[2].keyboard.c);
    device.digital("V",           virtualPorts[2].keyboard.v);
    device.digital("B",           virtualPorts[2].keyboard.b);
    device.digital("N",           virtualPorts[2].keyboard.n);
    device.digital("M",           virtualPorts[2].keyboard.m);
    device.digital(",",           virtualPorts[2].keyboard.comma);
    device.digital(".",           virtualPorts[2].keyboard.period);
    device.digital("/",           virtualPorts[2].keyboard.slash);
    device.digital("_",           virtualPorts[2].keyboard.ralt);
    device.digital("Right Shift", virtualPorts[2].keyboard.rshift);

    device.digital("Graph",       virtualPorts[2].keyboard.lalt);
    device.digital("Spacebar",    virtualPorts[2].keyboard.spacebar);

    device.digital("Home",        virtualPorts[2].keyboard.home);
    device.digital("Insert",      virtualPorts[2].keyboard.insert);
    device.digital("Delete",      virtualPorts[2].keyboard.delete_);

    device.digital("Up",          virtualPorts[2].keyboard.up);
    device.digital("Down",        virtualPorts[2].keyboard.down);
    device.digital("Left",        virtualPorts[2].keyboard.left);
    device.digital("Right",       virtualPorts[2].keyboard.right);

    port.append(device); }

    ports.append(port);
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

  string input = game->pak->attribute("input");
  if (input == "Family BASIC Keyboard") {
    if (auto port = root->find<ares::Node::Port>("Expansion Port")) {
      port->allocate("Family BASIC Keyboard");
      port->connect();
    }

    if (auto port = root->find<ares::Node::Port>("Expansion Port/Family BASIC Keyboard/Tape Port")) {
      port->allocate("Family BASIC Data Recorder");
      port->connect();
    }
  }

  return successful;
}

auto Famicom::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  if (familyBasicDataRecorder) {
    familyBasicDataRecorder->save(familyBasicDataRecorder->location);
  }
  return true;
}

auto Famicom::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Famicom") return system->pak;
  if(node->name() == "Famicom Cartridge") return game->pak;
  if(node->name() == "Family BASIC Data Recorder") return familyBasicDataRecorder->pak;
  return {};
}

auto Famicom::loadTape(ares::Node::Object node, string location) -> bool {
  if (node->name() == "Family BASIC Data Recorder") {
    familyBasicDataRecorder = mia::Medium::create("Tape");
    if (!location) {
      location = Emulator::load(familyBasicDataRecorder, settings.paths.home);
      if (!location) return false;
    }
    LoadResult result = familyBasicDataRecorder->load(location);
    if (result != successful) {
      familyBasicDataRecorder.reset();
      return false;
    }

    return true;
  }

  return false;
}

auto Famicom::unloadTape(ares::Node::Object node) -> void {
  if (node->name() == "Family BASIC Data Recorder") {
    familyBasicDataRecorder->save(familyBasicDataRecorder->location);
    familyBasicDataRecorder.reset();
  }
}