struct ColecoVision : Emulator {
  ColecoVision();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

ColecoVision::ColecoVision() {
  manufacturer = "Coleco";
  name = "ColecoVision";

  firmware.append({"BIOS", "World"});

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

    InputDevice device{"Gamepad"};
    device.button("Up",    virtualPads[id].up);
    device.button("Down",  virtualPads[id].down);
    device.button("Left",  virtualPads[id].left);
    device.button("Right", virtualPads[id].right);
    device.button("L",     virtualPads[id].select);
    device.button("R",     virtualPads[id].start);
    device.button("1",     virtualPads[id].a);
    device.button("2",     virtualPads[id].b);
    device.button("3",     virtualPads[id].c);
    device.button("4",     virtualPads[id].x);
    device.button("5",     virtualPads[id].y);
    device.button("6",     virtualPads[id].z);
    device.button("7",     virtualPads[id].l1);
    device.button("8",     virtualPads[id].r1);
    device.button("9",     virtualPads[id].l2);
    device.button("*",     virtualPads[id].r2);
    device.button("0",     virtualPads[id].lt);
    device.button("#",     virtualPads[id].rt);
    port.append(device);

    ports.append(port);
  }
}

auto ColecoVision::load() -> bool {
  game = mia::Medium::create("ColecoVision");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("ColecoVision");
  if(!system->load(firmware[0].location)) return errorFirmware(firmware[0]), false;

  auto region = Emulator::region();
  if(!ares::ColecoVision::load(root, {"[Coleco] ColecoVision (", region, ")"})) return false;

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

  return true;
}

auto ColecoVision::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto ColecoVision::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "ColecoVision") return system->pak;
  if(node->name() == "ColecoVision Cartridge") return game->pak;
  return {};
}

auto ColecoVision::input(ares::Node::Input::Input input) -> void {
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
    if(name == "Up"   ) mapping = virtualPads[*id].up;
    if(name == "Down" ) mapping = virtualPads[*id].down;
    if(name == "Left" ) mapping = virtualPads[*id].left;
    if(name == "Right") mapping = virtualPads[*id].right;
    if(name == "L"    ) mapping = virtualPads[*id].select;
    if(name == "R"    ) mapping = virtualPads[*id].start;
    if(name == "1"    ) mapping = virtualPads[*id].a;
    if(name == "2"    ) mapping = virtualPads[*id].b;
    if(name == "3"    ) mapping = virtualPads[*id].c;
    if(name == "4"    ) mapping = virtualPads[*id].x;
    if(name == "5"    ) mapping = virtualPads[*id].y;
    if(name == "6"    ) mapping = virtualPads[*id].z;
    if(name == "7"    ) mapping = virtualPads[*id].l1;
    if(name == "8"    ) mapping = virtualPads[*id].r1;
    if(name == "9"    ) mapping = virtualPads[*id].l2;
    if(name == "*"    ) mapping = virtualPads[*id].r2;
    if(name == "0"    ) mapping = virtualPads[*id].lt;
    if(name == "#"    ) mapping = virtualPads[*id].rt;

    if(mapping) {
      auto value = mapping->value();
      if(auto button = input->cast<ares::Node::Input::Button>()) {
        button->setValue(value);
      }
    }
  }
}
