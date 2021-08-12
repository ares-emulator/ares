struct PlayStation : Emulator {
  PlayStation();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;

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

  { InputDevice device{"Digital Gamepad"};
    device.digital("Up",       virtualPorts[id].pad.up);
    device.digital("Down",     virtualPorts[id].pad.down);
    device.digital("Left",     virtualPorts[id].pad.left);
    device.digital("Right",    virtualPorts[id].pad.right);
    device.digital("Cross",    virtualPorts[id].pad.b1);
    device.digital("Circle",   virtualPorts[id].pad.b2);
    device.digital("Square",   virtualPorts[id].pad.t1);
    device.digital("Triangle", virtualPorts[id].pad.t2);
    device.digital("L1",       virtualPorts[id].pad.l1);
    device.digital("L2",       virtualPorts[id].pad.l2);
    device.digital("R1",       virtualPorts[id].pad.r1);
    device.digital("R2",       virtualPorts[id].pad.r2);
    device.digital("Select",   virtualPorts[id].pad.select);
    device.digital("Start",    virtualPorts[id].pad.start);
    port.append(device); }
  
    // Skeleton dual-shock controller support
    // { InputDevice device{"Dual Shock Controller"};
    // device.digital("D-Pad Up",        virtualPorts[id].pad.up);
    // device.digital("D-Pad Down",      virtualPorts[id].pad.down);
    // device.digital("D-Pad Left",      virtualPorts[id].pad.left);
    // device.digital("D-Pad Right",     virtualPorts[id].pad.right);
    // device.analog ("LAnalog-Up",      virtualPorts[id].pad.lup);
    // device.analog ("LAnalog-Down",    virtualPorts[id].pad.ldown);
    // device.analog ("LAnalog-Left",    virtualPorts[id].pad.lleft);
    // device.analog ("LAnalog-Right",   virtualPorts[id].pad.lright);
    // device.digital("LAnalog-Thumb",   virtualPorts[id].pad.lthumbl);
    // device.analog ("RAnalog-Up",      virtualPorts[id].pad.lup);
    // device.analog ("RAnalog-Down",    virtualPorts[id].pad.ldown);
    // device.analog ("RAnalog-Left",    virtualPorts[id].pad.lleft);
    // device.analog ("RAnalog-Right",   virtualPorts[id].pad.lright);
    // device.digital("RAnalog-Thumb",   virtualPorts[id].pad.rthumbl);
    // device.digital("Cross",           virtualPorts[id].pad.b1);
    // device.digital("Circle",          virtualPorts[id].pad.b2);
    // device.digital("Square",          virtualPorts[id].pad.t1);
    // device.digital("Triangle",        virtualPorts[id].pad.t2);
    // device.digital("L1",              virtualPorts[id].pad.l1);
    // device.digital("L2",              virtualPorts[id].pad.l2);
    // device.digital("R1",              virtualPorts[id].pad.r1);
    // device.digital("R2",              virtualPorts[id].pad.r2);
    // device.digital("Select",          virtualPorts[id].pad.select);
    // device.digital("Start",           virtualPorts[id].pad.start);
    // device.digital("Analog (button)", virtualPorts[id].pad.mode);
    // device.rumble ("Rumble",          virtualPorts[id].pad.rumble);
    // port.append(device); }

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
