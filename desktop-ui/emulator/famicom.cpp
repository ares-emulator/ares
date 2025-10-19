struct FamilyBasicKeyboard : InputDevice {
  FamilyBasicKeyboard();

  auto loadSetting(Markup::Node *node) -> void;
  auto saveSetting(Markup::Node *node) -> void;

  InputDigital f1, f2, f3, f4, f5, f6, f7, f8;
  InputDigital num1, num2, num3, num4, num5, num6, num7, num8, num9, num0, minus, power, yen, stop;
  InputDigital esc, q, w, e, r, t, y, u, i, o, p, at, lbrace, enter;
  InputDigital control, a, s, d, f, g, h, j, k, l, semicolon, colon, rbrace, kana;
  InputDigital lshift, z, x, c, v, b, n, m, comma, period, slash, underscore, rshift;
  InputDigital graph, spacebar;
  InputDigital home, insert, backspace;
  InputDigital up, down, left, right;
};

FamilyBasicKeyboard::FamilyBasicKeyboard() : InputDevice("Family BASIC Keyboard") {
  InputDevice::digital("F1", f1);
  InputDevice::digital("F2", f2);
  InputDevice::digital("F3", f3);
  InputDevice::digital("F4", f4);
  InputDevice::digital("F5", f5);
  InputDevice::digital("F6", f6);
  InputDevice::digital("F7", f7);
  InputDevice::digital("F8", f8);

  InputDevice::digital("Num1",  num1);
  InputDevice::digital("Num2",  num2);
  InputDevice::digital("Num3",  num3);
  InputDevice::digital("Num4",  num4);
  InputDevice::digital("Num5",  num5);
  InputDevice::digital("Num6",  num6);
  InputDevice::digital("Num7",  num7);
  InputDevice::digital("Num8",  num8);
  InputDevice::digital("Num9",  num9);
  InputDevice::digital("Num0",  num0);
  InputDevice::digital("Minus", minus);
  InputDevice::digital("Power", power);
  InputDevice::digital("Yen",   yen);
  InputDevice::digital("Stop",  stop);

  InputDevice::digital("Escape",     esc);
  InputDevice::digital("Q",          q);
  InputDevice::digital("W",          w);
  InputDevice::digital("E",          e);
  InputDevice::digital("R",          r);
  InputDevice::digital("T",          t);
  InputDevice::digital("Y",          y);
  InputDevice::digital("U",          u);
  InputDevice::digital("I",          i);
  InputDevice::digital("O",          o);
  InputDevice::digital("P",          p);
  InputDevice::digital("At",         at);
  InputDevice::digital("Left Brace", lbrace);
  InputDevice::digital("Return",     enter);

  InputDevice::digital("Control",     control);
  InputDevice::digital("A",           a);
  InputDevice::digital("S",           s);
  InputDevice::digital("D",           d);
  InputDevice::digital("F",           f);
  InputDevice::digital("G",           g);
  InputDevice::digital("H",           h);
  InputDevice::digital("J",           j);
  InputDevice::digital("K",           k);
  InputDevice::digital("L",           l);
  InputDevice::digital("Semicolon",   semicolon);
  InputDevice::digital("Colon",       colon);
  InputDevice::digital("Right Brace", rbrace);
  InputDevice::digital("Kana",        kana);

  InputDevice::digital("Left Shift",  lshift);
  InputDevice::digital("Z",           z);
  InputDevice::digital("X",           x);
  InputDevice::digital("C",           c);
  InputDevice::digital("V",           v);
  InputDevice::digital("B",           b);
  InputDevice::digital("N",           n);
  InputDevice::digital("M",           m);
  InputDevice::digital("Comma",       comma);
  InputDevice::digital("Period",      period);
  InputDevice::digital("Slash",       slash);
  InputDevice::digital("Underscore",  underscore);
  InputDevice::digital("Right Shift", rshift);

  InputDevice::digital("Graph",    graph);
  InputDevice::digital("Spacebar", spacebar);

  InputDevice::digital("Home",      home);
  InputDevice::digital("Insert",    insert);
  InputDevice::digital("Backspace", backspace);

  InputDevice::digital("Up",    up);
  InputDevice::digital("Down",  down);
  InputDevice::digital("Left",  left);
  InputDevice::digital("Right", right);
}

auto FamilyBasicKeyboard::loadSetting(Markup::Node *node) -> void {
  for (auto& input : inputs) {
    string node_name = string{"Famicom/", name, "/", input.name}.replace(" ", ".");
    if (auto subnode = (*node)[node_name]) {
      string value = subnode.string();
      auto parts = nall::split(value, ";");
      parts.resize(BindingLimit);
      for (u32 binding : range(BindingLimit)) {
        input.mapping->assignments[binding] = parts[binding];
      }
    }
  }
}

auto FamilyBasicKeyboard::saveSetting(Markup::Node *node) -> void {
  for (auto& input : inputs) {
    string node_name = string{"Famicom/", name, "/", input.name}.replace(" ", ".");
    string value;
    for (auto& assignment : input.mapping->assignments) {
      value.append(assignment, ";");
    }
    value.trimRight(";", 1L);

    (*node)(node_name).setValue(value);
  }
}

struct Famicom : Emulator {
  Famicom();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;

  auto loadTape(ares::Node::Object node, string location) -> bool override;
  auto unloadTape(ares::Node::Object node) -> void override;

  auto loadSetting(Markup::Node *node) -> void override;
  auto saveSetting(Markup::Node *node) -> void override;
  auto bindInput() -> void override;

  FamilyBasicKeyboard familyBasicKeyboard;
  std::shared_ptr<mia::Pak> familyBasicDataRecorder{};
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

  {
    InputPort port{"Expansion Port"};
    port.append(familyBasicKeyboard);
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

auto Famicom::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
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

auto Famicom::loadSetting(Markup::Node *node) -> void {
  familyBasicKeyboard.loadSetting(node);
}

auto Famicom::saveSetting(Markup::Node *node) -> void {
  familyBasicKeyboard.saveSetting(node);
}

auto Famicom::bindInput() -> void {
  for (auto& input : familyBasicKeyboard.inputs) {
    input.mapping->bind();
  }
}
