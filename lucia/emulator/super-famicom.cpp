namespace ares::SuperFamicom {
  auto load(Node::System& node, string name) -> bool;
}

struct SuperFamicom : Emulator {
  SuperFamicom();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto input(ares::Node::Input::Input) -> void override;

  struct Pak {
    string location;
    shared_pointer<vfs::directory> pak;
  } gb, bs, stA, stB;
};

SuperFamicom::SuperFamicom() {
  medium = mia::medium("Super Famicom");
  manufacturer = "Nintendo";
  name = "Super Famicom";
}

auto SuperFamicom::load() -> bool {
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
        if(gb.pak = mia::medium("Game Boy")->pak(location)) {
          gb.location = location;
          slot->allocate();
          slot->connect();
        }
      }
    }
    #endif

    if(auto slot = cartridge->find<ares::Node::Port>("BS Memory Slot")) {
      if(auto location = program.load(mia::medium("BS Memory"), settings.paths.superFamicom.bsMemory)) {
        if(bs.pak = mia::medium("BS Memory")->pak(location)) {
          bs.location = location;
          slot->allocate();
          slot->connect();
        }
      }
    }

    if(auto slot = cartridge->find<ares::Node::Port>("Sufami Turbo Slot A")) {
      if(auto location = program.load(mia::medium("Sufami Turbo"), settings.paths.superFamicom.sufamiTurbo)) {
        if(stA.pak = mia::medium("Sufami Turbo")->pak(location)) {
          stA.location = location;
          slot->allocate();
          slot->connect();
        }
      }
    }

    if(auto slot = cartridge->find<ares::Node::Port>("Sufami Turbo Slot B")) {
      if(auto location = program.load(mia::medium("Sufami Turbo"), settings.paths.superFamicom.sufamiTurbo)) {
        if(stB.pak = mia::medium("Sufami Turbo")->pak(location)) {
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

auto SuperFamicom::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(node->name() == "Super Famicom") {
    if(name == "boards.bml") return vfs::memory::open({Resource::SuperFamicom::Boards, sizeof Resource::SuperFamicom::Boards});
    if(name == "ipl.rom"   ) return vfs::memory::open({Resource::SuperFamicom::IPLROM, sizeof Resource::SuperFamicom::IPLROM});
  }

  if(node->name() == "Super Famicom") {
    if(auto fp = pak->find(name)) return fp;
    if(auto fp = Emulator::save(name, mode, "save.ram",           ".sav"    )) return fp;
    if(auto fp = Emulator::save(name, mode, "download.ram",       ".psr"    )) return fp;
    if(auto fp = Emulator::save(name, mode, "time.rtc",           ".rtc"    )) return fp;
    if(auto fp = Emulator::save(name, mode, "upd7725.data.ram",   ".dsp.sav")) return fp;
    if(auto fp = Emulator::save(name, mode, "upd96050.data.ram",  ".dsp.sav")) return fp;
    if(auto fp = Emulator::save(name, mode, "hg51bs169.data.ram", ".cx4.sav")) return fp;
    if(auto fp = Emulator::save(name, mode, "arm6.data.ram",      ".arm.sav")) return fp;
    if(name == "msu1/data.rom") {
      auto location = locate(game.location, ".msu");
      if(auto result = vfs::disk::open(location, mode)) return result;
    }
    if(name.beginsWith("msu1/track-")) {
      auto location = locate({game.location, string{name}.trimLeft("msu1/", 1L)}, ".pcm");
      if(auto result = vfs::disk::open(location, mode)) return result;
    }
  }

  if(node->name() == "Super Game Boy") {
    if(auto fp = pak->find(name)) return fp;
  }

  if(node->name() == "Game Boy") {
    if(auto fp = gb.pak->find(name)) return fp;
    if(auto fp = Emulator::save(name, mode, "save.ram",       ".sav", node->name(), gb.location)) return fp;
    if(auto fp = Emulator::save(name, mode, "save.eeprom",    ".sav", node->name(), gb.location)) return fp;
    if(auto fp = Emulator::save(name, mode, "download.flash", ".sav", node->name(), gb.location)) return fp;
    if(auto fp = Emulator::save(name, mode, "time.rtc",       ".rtc", node->name(), gb.location)) return fp;
  }

  if(node->name() == "BS Memory") {
    if(auto fp = Emulator::save(name, mode, "program.flash", ".flash", node->name(), bs.location)) return fp;
    if(auto fp = bs.pak->find(name)) return fp;
  }

  if(node->name() == "Sufami Turbo") {
    if(auto parent = node->parent()) {
      if(auto port = parent.acquire()) {
        if(port->name() == "Sufami Turbo Slot A") {
          if(auto fp = stA.pak->find(name)) return fp;
          if(auto fp = Emulator::save(name, mode, "save.ram", ".slotA.sav", node->name(), stA.location)) return fp;
        }
        if(port->name() == "Sufami Turbo Slot B") {
          if(auto fp = stB.pak->find(name)) return fp;
          if(auto fp = Emulator::save(name, mode, "save.ram", ".slotB.sav", node->name(), stB.location)) return fp;
        }
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
