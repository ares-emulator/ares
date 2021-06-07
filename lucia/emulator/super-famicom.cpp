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

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

    InputDevice device{"Gamepad"};
    device.button("Up",     virtualPads[id].up);
    device.button("Down",   virtualPads[id].down);
    device.button("Left",   virtualPads[id].left);
    device.button("Right",  virtualPads[id].right);
    device.button("B",      virtualPads[id].a);
    device.button("A",      virtualPads[id].b);
    device.button("Y",      virtualPads[id].x);
    device.button("X",      virtualPads[id].y);
    device.button("L",      virtualPads[id].l1);
    device.button("R",      virtualPads[id].r1);
    device.button("Select", virtualPads[id].select);
    device.button("Start",  virtualPads[id].start);
    port.append(device);

    ports.append(port);
  }
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

auto SuperFamicom::input(ares::Node::Input::Input input) -> void {
  auto device = ares::Node::parent(input);
  if(!device) return;

  auto port = ares::Node::parent(device);
  if(!port) return;

  maybe<u32> id;
  if(port->name() == "Controller Port 1") id = 0;
  if(port->name() == "Controller Port 2") id = 1;
  if(!id) return;

  if(device->name() == "Gamepad") {
    auto name = input->name();
    maybe<InputMapping&> mapping;
    if(name == "Up"    ) mapping = virtualPads[*id].up;
    if(name == "Down"  ) mapping = virtualPads[*id].down;
    if(name == "Left"  ) mapping = virtualPads[*id].left;
    if(name == "Right" ) mapping = virtualPads[*id].right;
    if(name == "B"     ) mapping = virtualPads[*id].a;
    if(name == "A"     ) mapping = virtualPads[*id].b;
    if(name == "Y"     ) mapping = virtualPads[*id].x;
    if(name == "X"     ) mapping = virtualPads[*id].y;
    if(name == "L"     ) mapping = virtualPads[*id].l1;
    if(name == "R"     ) mapping = virtualPads[*id].r1;
    if(name == "Select") mapping = virtualPads[*id].select;
    if(name == "Start" ) mapping = virtualPads[*id].start;

    if(mapping) {
      auto value = mapping->value();
      if(auto button = input->cast<ares::Node::Input::Button>()) {
        button->setValue(value);
      }
    }
  }
}
