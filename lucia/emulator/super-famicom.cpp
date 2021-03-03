struct SuperFamicom : Emulator {
  SuperFamicom();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;

  Pak gb, bs, stA, stB;
};

SuperFamicom::SuperFamicom() {
  medium = mia::medium("Super Famicom");
  manufacturer = "Nintendo";
  name = "Super Famicom";
}

auto SuperFamicom::load() -> bool {
  system.pak->append("boards.bml", {Resource::SuperFamicom::Boards, sizeof Resource::SuperFamicom::Boards});
  system.pak->append("ipl.rom",    {Resource::SuperFamicom::IPLROM, sizeof Resource::SuperFamicom::IPLROM});

  auto region = Emulator::region();
  if(region.beginsWith("NTSC")
  || region.endsWith("BRA")
  || region.endsWith("CAN")
  || region.endsWith("HKG")
  || region.endsWith("JPN")
  || region.endsWith("KOR")
  || region.endsWith("LTN")
  || region.endsWith("ROC")
  || region.endsWith("USA")
  || region.beginsWith("SHVC-")
  ) {
    region = "NTSC";
  } else {
    region = "PAL";
  }
  if(!ares::SuperFamicom::load(root, {"[Nintendo] Super Famicom (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    auto cartridge = port->allocate();
    port->connect();

    #if defined(CORE_GB)
    if(auto slot = cartridge->find<ares::Node::Port>("Super Game Boy/Cartridge Slot")) {
      if(auto location = program.load(mia::medium("Game Boy"), settings.paths.superFamicom.gameBoy)) {
        if(gb.pak = mia::medium("Game Boy")->load(location)) {
          gb.location = location;
          slot->allocate();
          slot->connect();
        } else if(gb.pak = mia::medium("Game Boy Color")->load(location)) {
          gb.location = location;
          slot->allocate();
          slot->connect();
        }
      }
    }
    #endif

    if(auto slot = cartridge->find<ares::Node::Port>("BS Memory Slot")) {
      if(auto location = program.load(mia::medium("BS Memory"), settings.paths.superFamicom.bsMemory)) {
        if(bs.pak = mia::medium("BS Memory")->load(location)) {
          bs.location = location;
          slot->allocate();
          slot->connect();
        }
      }
    }

    if(auto slot = cartridge->find<ares::Node::Port>("Sufami Turbo Slot A")) {
      if(auto location = program.load(mia::medium("Sufami Turbo"), settings.paths.superFamicom.sufamiTurbo)) {
        if(stA.pak = mia::medium("Sufami Turbo")->load(location)) {
          stA.location = location;
          slot->allocate();
          slot->connect();
        }
      }
    }

    if(auto slot = cartridge->find<ares::Node::Port>("Sufami Turbo Slot B")) {
      if(auto location = program.load(mia::medium("Sufami Turbo"), settings.paths.superFamicom.sufamiTurbo)) {
        if(stB.pak = mia::medium("Sufami Turbo")->load(location)) {
          stB.location = location;
          slot->allocate();
          slot->connect();
        }
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
  medium->save(game.location, game.pak);
  if(gb.pak) mia::medium("Game Boy")->save(gb.location, gb.pak);
  if(bs.pak) mia::medium("BS Memory")->save(bs.location, bs.pak);
  if(stA.pak) mia::medium("Sufami Turbo")->save(stA.location, stA.pak);
  if(stB.pak) mia::medium("Sufami Turbo")->save(stB.location, stB.pak);
  return true;
}

auto SuperFamicom::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Super Famicom") return system.pak;
  if(node->name() == "Super Famicom Cartridge") return game.pak;
  if(node->name() == "Game Boy Cartridge") return gb.pak;
  if(node->name() == "Game Boy Color Cartridge") return gb.pak;
  if(node->name() == "BS Memory Cartridge") return bs.pak;
  if(node->name() == "Sufami Turbo Cartridge") {
    if(auto parent = node->parent()) {
      if(auto port = parent.acquire()) {
        if(port->name() == "Sufami Turbo Slot A") return stA.pak;
        if(port->name() == "Sufami Turbo Slot B") return stB.pak;
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
