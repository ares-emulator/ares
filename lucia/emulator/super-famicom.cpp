struct SuperFamicom : Emulator {
  SuperFamicom();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;

  shared_pointer<mia::Pak> gb, bs, stA, stB;
};

SuperFamicom::SuperFamicom() {
  manufacturer = "Nintendo";
  name = "Super Famicom";
}

auto SuperFamicom::load() -> bool {
  game = mia::Medium::create("Super Famicom");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("Super Famicom");
  if(!system->load()) return false;

  auto region = Emulator::region();
  if(!ares::SuperFamicom::load(root, {"[Nintendo] Super Famicom (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    auto cartridge = port->allocate();
    port->connect();

    if(auto slot = cartridge->find<ares::Node::Port>("Super Game Boy/Cartridge Slot")) {
      gb = mia::Medium::create("Game Boy");
      if(gb->load(Emulator::load(gb, settings.paths.superFamicom.gameBoy))) {
        slot->allocate();
        slot->connect();
      } else {
        gb.reset();
      }
    }

    if(auto slot = cartridge->find<ares::Node::Port>("BS Memory Slot")) {
      bs = mia::Medium::create("BS Memory");
      if(bs->load(Emulator::load(bs, settings.paths.superFamicom.bsMemory))) {
        slot->allocate();
        slot->connect();
      } else {
        bs.reset();
      }
    }

    if(auto slot = cartridge->find<ares::Node::Port>("Sufami Turbo Slot A")) {
      stA = mia::Medium::create("Sufami Turbo");
      if(stA->load(Emulator::load(stA, settings.paths.superFamicom.sufamiTurbo))) {
        slot->allocate();
        slot->connect();
      } else {
        stA.reset();
      }
    }

    if(auto slot = cartridge->find<ares::Node::Port>("Sufami Turbo Slot B")) {
      stB = mia::Medium::create("Sufami Turbo");
      if(stB->load(Emulator::load(stB, settings.paths.superFamicom.sufamiTurbo))) {
        slot->allocate();
        slot->connect();
      } else {
        stB.reset();
      }
    }
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Gamepad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return true;
}

auto SuperFamicom::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  if(gb) gb->save(gb->location);
  if(bs) bs->save(bs->location);
  if(stA) stA->save(stA->location);
  if(stB) stB->save(stB->location);
  return true;
}

auto SuperFamicom::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Super Famicom") return system->pak;
  if(node->name() == "Super Famicom Cartridge") return game->pak;
  if(node->name() == "Game Boy Cartridge") return gb->pak;
  if(node->name() == "Game Boy Color Cartridge") return gb->pak;
  if(node->name() == "BS Memory Cartridge") return bs->pak;
  if(node->name() == "Sufami Turbo Cartridge") {
    if(auto parent = node->parent()) {
      if(auto port = parent.acquire()) {
        if(port->name() == "Sufami Turbo Slot A") return stA->pak;
        if(port->name() == "Sufami Turbo Slot B") return stB->pak;
      }
    }
  }
  return {};
}

auto SuperFamicom::input(ares::Node::Input::Input node) -> void {
  auto parent = ares::Node::parent(node);
  if(!parent) return;

  auto port = ares::Node::parent(parent);
  if(!port) return;

  maybe<u32> index;
  if(port->name() == "Controller Port 1") index = 0;
  if(port->name() == "Controller Port 2") index = 1;
  if(!index) return;

  if(parent->name() == "Gamepad") {
    auto name = node->name();
    maybe<InputMapping&> mapping;
    if(name == "Up"    ) mapping = virtualPads[*index].up;
    if(name == "Down"  ) mapping = virtualPads[*index].down;
    if(name == "Left"  ) mapping = virtualPads[*index].left;
    if(name == "Right" ) mapping = virtualPads[*index].right;
    if(name == "B"     ) mapping = virtualPads[*index].a;
    if(name == "A"     ) mapping = virtualPads[*index].b;
    if(name == "Y"     ) mapping = virtualPads[*index].x;
    if(name == "X"     ) mapping = virtualPads[*index].y;
    if(name == "L"     ) mapping = virtualPads[*index].l1;
    if(name == "R"     ) mapping = virtualPads[*index].r1;
    if(name == "Select") mapping = virtualPads[*index].select;
    if(name == "Start" ) mapping = virtualPads[*index].start;

    if(mapping) {
      auto value = mapping->value();
      if(auto button = node->cast<ares::Node::Input::Button>()) {
        button->setValue(value);
      }
    }
  }
}
