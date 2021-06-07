struct Mega32X : Emulator {
  Mega32X();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;

  shared_pointer<mia::Pak> disc;
  u32 regionID = 0;
};

Mega32X::Mega32X() {
  manufacturer = "Sega";
  name = "Mega 32X";

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

    InputDevice device{"Fighting Pad"};
    device.button("Up",    virtualPads[id].up);
    device.button("Down",  virtualPads[id].down);
    device.button("Left",  virtualPads[id].left);
    device.button("Right", virtualPads[id].right);
    device.button("A",     virtualPads[id].a);
    device.button("B",     virtualPads[id].b);
    device.button("C",     virtualPads[id].c);
    device.button("X",     virtualPads[id].x);
    device.button("Y",     virtualPads[id].y);
    device.button("Z",     virtualPads[id].z);
    device.button("Mode",  virtualPads[id].select);
    device.button("Start", virtualPads[id].start);
    port.append(device);

    ports.append(port);
  }
}

auto Mega32X::load() -> bool {
  game = mia::Medium::create("Mega 32X");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "PAL"   ) regionID = 2;
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  string name;
  if(game->pak->attribute("megacd").boolean()) {
    //use Mega CD firmware settings
    vector<Firmware> firmware;
    for(auto& emulator : emulators) {
      if(emulator->name == "Mega CD") firmware = emulator->firmware;
    }
    if(!firmware) return false;  //should never occur
    name = "Mega CD 32X";
    system = mia::System::create("Mega CD 32X");
    if(!system->load(firmware[regionID].location)) return errorFirmware(firmware[regionID], "Mega CD"), false;

    disc = mia::Medium::create("Mega CD");
    if(!disc->load(Emulator::load(disc, configuration.game))) disc.reset();
  } else {
    name = "Mega 32X";
    system = mia::System::create("Mega 32X");
    if(!system->load()) return false;
  }

  if(!ares::MegaDrive::load(root, {"[Sega] ", name, " (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Mega CD/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Fighting Pad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Fighting Pad");
    port->connect();
  }

  return true;
}

auto Mega32X::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  if(disc) disc->save(disc->location);
  return true;
}

auto Mega32X::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Mega Drive") return system->pak;
  if(node->name() == "Mega Drive Cartridge") return game->pak;
  if(node->name() == "Mega CD Disc" && disc) return disc->pak;
  return {};
}

auto Mega32X::input(ares::Node::Input::Input input) -> void {
  auto device = ares::Node::parent(input);
  if(!device) return;

  auto port = ares::Node::parent(device);
  if(!port) return;

  maybe<u32> id;
  if(port->name() == "Controller Port 1") id = 0;
  if(port->name() == "Controller Port 2") id = 1;
  if(!id) return;

  if(device->name() == "Fighting Pad") {
    auto name = input->name();
    maybe<InputMapping&> mapping;
    if(name == "Up"   ) mapping = virtualPads[*id].up;
    if(name == "Down" ) mapping = virtualPads[*id].down;
    if(name == "Left" ) mapping = virtualPads[*id].left;
    if(name == "Right") mapping = virtualPads[*id].right;
    if(name == "A"    ) mapping = virtualPads[*id].a;
    if(name == "B"    ) mapping = virtualPads[*id].b;
    if(name == "C"    ) mapping = virtualPads[*id].c;
    if(name == "X"    ) mapping = virtualPads[*id].x;
    if(name == "Y"    ) mapping = virtualPads[*id].y;
    if(name == "Z"    ) mapping = virtualPads[*id].z;
    if(name == "Mode" ) mapping = virtualPads[*id].select;
    if(name == "Start") mapping = virtualPads[*id].start;

    if(mapping) {
      auto value = mapping->value();
      if(auto button = input->cast<ares::Node::Input::Button>()) {
        button->setValue(value);
      }
    }
  }
}
