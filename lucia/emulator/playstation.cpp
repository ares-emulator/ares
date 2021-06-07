struct PlayStation : Emulator {
  PlayStation();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;

  shared_pointer<mia::Pak> memoryCard;
  u32 regionID = 0;
};

PlayStation::PlayStation() {
  manufacturer = "Sony";
  name = "PlayStation";

  firmware.append({"BIOS", "US"});      //NTSC-U
  firmware.append({"BIOS", "Japan"});   //NTSC-J
  firmware.append({"BIOS", "Europe"});  //PAL

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

    InputDevice device{"Digital Gamepad"};
    device.button("Up",       virtualPads[id].up);
    device.button("Down",     virtualPads[id].down);
    device.button("Left",     virtualPads[id].left);
    device.button("Right",    virtualPads[id].right);
    device.button("Cross",    virtualPads[id].a);
    device.button("Circle",   virtualPads[id].b);
    device.button("Square",   virtualPads[id].x);
    device.button("Triangle", virtualPads[id].y);
    device.button("L1",       virtualPads[id].l1);
    device.button("L2",       virtualPads[id].l2);
    device.button("R1",       virtualPads[id].r1);
    device.button("R2",       virtualPads[id].r2);
    device.button("Select",   virtualPads[id].select);
    device.button("Start",    virtualPads[id].start);
    port.append(device);

    ports.append(port);
  }
}

auto PlayStation::load() -> bool {
  game = mia::Medium::create("PlayStation");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  auto region = Emulator::region();
  //if statements below are ordered by lowest to highest priority
  if(region == "PAL"   ) regionID = 2;
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  system = mia::System::create("PlayStation");
  if(!system->load(firmware[regionID].location)) return errorFirmware(firmware[regionID]), false;

  if(!ares::PlayStation::load(root, {"[Sony] PlayStation (", region, ")"})) return false;

  if(auto fastBoot = root->find<ares::Node::Setting::Boolean>("Fast Boot")) {
    fastBoot->setValue(settings.boot.fast);
  }

  if(auto port = root->find<ares::Node::Port>("PlayStation/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Digital Gamepad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Memory Card Port 1")) {
    memoryCard = mia::Pak::create("PlayStation");
    memoryCard->pak->append("save.card", 128_KiB);
    memoryCard->load("save.card", ".card", game->location);
    port->allocate("Memory Card");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Digital Gamepad");
    port->connect();
  }

  return true;
}

auto PlayStation::save() -> bool {
  root->save();
  system->save(game->location);
  game->save(game->location);
  if(memoryCard) memoryCard->save("save.card", ".card", game->location);
  return true;
}

auto PlayStation::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "PlayStation") return system->pak;
  if(node->name() == "PlayStation Disc") return game->pak;
  if(node->name() == "Memory Card") return memoryCard->pak;
  return {};
}

auto PlayStation::input(ares::Node::Input::Input input) -> void {
  auto device = ares::Node::parent(input);
  if(!device) return;

  auto port = ares::Node::parent(device);
  if(!port) return;

  maybe<u32> id;
  if(port->name() == "Controller Port 1") id = 0;
  if(port->name() == "Controller Port 2") id = 1;
  if(!id) return;

  if(device->name() == "Digital Gamepad") {
    auto name = input->name();
    maybe<InputMapping&> mapping;
    if(name == "Up"      ) mapping = virtualPads[*id].up;
    if(name == "Down"    ) mapping = virtualPads[*id].down;
    if(name == "Left"    ) mapping = virtualPads[*id].left;
    if(name == "Right"   ) mapping = virtualPads[*id].right;
    if(name == "Cross"   ) mapping = virtualPads[*id].a;
    if(name == "Circle"  ) mapping = virtualPads[*id].b;
    if(name == "Square"  ) mapping = virtualPads[*id].x;
    if(name == "Triangle") mapping = virtualPads[*id].y;
    if(name == "L1"      ) mapping = virtualPads[*id].l1;
    if(name == "L2"      ) mapping = virtualPads[*id].l2;
    if(name == "R1"      ) mapping = virtualPads[*id].r1;
    if(name == "R2"      ) mapping = virtualPads[*id].r2;
    if(name == "Select"  ) mapping = virtualPads[*id].select;
    if(name == "Start"   ) mapping = virtualPads[*id].start;

    if(mapping) {
      auto value = mapping->value();
      if(auto axis = input->cast<ares::Node::Input::Axis>()) {
        axis->setValue(value);
      }
      if(auto button = input->cast<ares::Node::Input::Button>()) {
        button->setValue(value);
      }
    }
  }
}
