struct Nintendo64 : Emulator {
  Nintendo64();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

  shared_pointer<mia::Pak> gamepad;
};

Nintendo64::Nintendo64() {
  manufacturer = "Nintendo";
  name = "Nintendo 64";

  for(auto id : range(4)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.analog ("L-Up",    virtualPorts[id].pad.lup);
    device.analog ("L-Down",  virtualPorts[id].pad.ldown);
    device.analog ("L-Left",  virtualPorts[id].pad.lleft);
    device.analog ("L-Right", virtualPorts[id].pad.lright);
    device.digital("Up",      virtualPorts[id].pad.up);
    device.digital("Down",    virtualPorts[id].pad.down);
    device.digital("Left",    virtualPorts[id].pad.left);
    device.digital("Right",   virtualPorts[id].pad.right);
    device.digital("B",       virtualPorts[id].pad.a);
    device.digital("A",       virtualPorts[id].pad.b);
    device.digital("C-Up",    virtualPorts[id].pad.rup);
    device.digital("C-Down",  virtualPorts[id].pad.rdown);
    device.digital("C-Left",  virtualPorts[id].pad.rleft);
    device.digital("C-Right", virtualPorts[id].pad.rright);
    device.digital("L",       virtualPorts[id].pad.l1);
    device.digital("R",       virtualPorts[id].pad.r1);
    device.digital("Z",       virtualPorts[id].pad.z);
    device.digital("Start",   virtualPorts[id].pad.start);
    device.rumble ("Rumble",  virtualPorts[id].pad.rumble);
    device.analog ("X-Axis",  virtualPorts[id].pad.lleft, virtualPorts[id].pad.lright);
    device.analog ("Y-Axis",  virtualPorts[id].pad.lup,   virtualPorts[id].pad.ldown);
    port.append(device); }

    ports.append(port);
  }
}

auto Nintendo64::load() -> bool {
  game = mia::Medium::create("Nintendo 64");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("Nintendo 64");
  if(!system->load()) return false;

  ares::Nintendo64::option("Quality", settings.video.quality);
  ares::Nintendo64::option("Supersampling", settings.video.supersampling);

  auto region = Emulator::region();
  if(!ares::Nintendo64::load(root, {"[Nintendo] Nintendo 64 (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  auto controllers = 4;
  //Jeopardy! does not accept any input if > 3 controllers are plugged in at boot.
  if(game->pak->attribute("id") == "NJOE") controllers = min(controllers, 3);

  for(auto id : range(controllers)) {
    if(auto port = root->find<ares::Node::Port>({"Controller Port ", 1 + id})) {
      auto peripheral = port->allocate("Gamepad");
      port->connect();
      if(auto port = peripheral->find<ares::Node::Port>("Pak")) {
        if(id == 0 && game->pak->attribute("mempak").boolean()) {
          gamepad = mia::Pak::create("Nintendo 64");
          gamepad->pak->append("save.pak", 32_KiB);
          gamepad->load("save.pak", ".pak", game->location);
          port->allocate("Controller Pak");
          port->connect();
        } else if(game->pak->attribute("rumble").boolean()) {
          port->allocate("Rumble Pak");
          port->connect();
        }
      }
    }
  }

  return true;
}

auto Nintendo64::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  if(gamepad) gamepad->save("save.pak", ".pak", game->location);
  return true;
}

auto Nintendo64::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Nintendo 64") return system->pak;
  if(node->name() == "Nintendo 64 Cartridge") return game->pak;
  if(node->name() == "Gamepad") return gamepad->pak;
  return {};
}
