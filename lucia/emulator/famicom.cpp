struct Famicom : Emulator {
  Famicom();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

Famicom::Famicom() {
  manufacturer = "Nintendo";
  name = "Famicom";

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

    InputDevice device{"Gamepad"};
    device.button("Up",         virtualPads[id].up);
    device.button("Down",       virtualPads[id].down);
    device.button("Left",       virtualPads[id].left);
    device.button("Right",      virtualPads[id].right);
    device.button("B",          virtualPads[id].a);
    device.button("A",          virtualPads[id].b);
    device.button("Select",     virtualPads[id].select);
    device.button("Start",      virtualPads[id].start);
    device.button("Microphone", virtualPads[id].x);
    port.append(device);

    ports.append(port);
  }
}

auto Famicom::load() -> bool {
  game = mia::Medium::create("Famicom");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("Famicom");
  if(!system->load()) return false;

  auto region = Emulator::region();
  if(!ares::Famicom::load(root, {"[Nintendo] Famicom (", region, ")"})) return false;

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

auto Famicom::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto Famicom::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Famicom") return system->pak;
  if(node->name() == "Famicom Cartridge") return game->pak;
  return {};
}

auto Famicom::input(ares::Node::Input::Input input) -> void {
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
    if(name == "Up"        ) mapping = virtualPads[*id].up;
    if(name == "Down"      ) mapping = virtualPads[*id].down;
    if(name == "Left"      ) mapping = virtualPads[*id].left;
    if(name == "Right"     ) mapping = virtualPads[*id].right;
    if(name == "B"         ) mapping = virtualPads[*id].a;
    if(name == "A"         ) mapping = virtualPads[*id].b;
    if(name == "Select"    ) mapping = virtualPads[*id].select;
    if(name == "Start"     ) mapping = virtualPads[*id].start;
    if(name == "Microphone") mapping = virtualPads[*id].x;

    if(mapping) {
      auto value = mapping->value();
      if(auto button = input->cast<ares::Node::Input::Button>()) {
         button->setValue(value);
      }
    }
  }
}
