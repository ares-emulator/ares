struct FamicomDiskSystem : Emulator {
  FamicomDiskSystem();
  auto load() -> LoadResult override;
  auto unload() -> void override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
  auto notify(const string& message) -> void override;

  std::shared_ptr<mia::Pak> bios;
};

FamicomDiskSystem::FamicomDiskSystem() {
  manufacturer = "Nintendo";
  name = "Famicom Disk System";

  firmware.push_back({"BIOS", "Japan", "fdc1a76e654feea993fcb38366e05ee5f4eb641f86fe6bebaeefd412e112dd72"});

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",         virtualPorts[id].pad.up);
    device.digital("Down",       virtualPorts[id].pad.down);
    device.digital("Left",       virtualPorts[id].pad.left);
    device.digital("Right",      virtualPorts[id].pad.right);
    device.digital("B",          virtualPorts[id].pad.west);
    device.digital("A",          virtualPorts[id].pad.south);
    device.digital("Select",     virtualPorts[id].pad.select);
    device.digital("Start",      virtualPorts[id].pad.start);
    device.digital("Microphone", virtualPorts[id].pad.north);
    port.append(device); }

    ports.push_back(port);
  }
}

auto FamicomDiskSystem::load() -> LoadResult {
  game = std::dynamic_pointer_cast<mia::Pak>(mia::Medium::create("Famicom Disk System"));
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  bios = std::dynamic_pointer_cast<mia::Pak>(mia::Medium::create("Famicom"));
  result = bios->load(firmware[0].location);
  if(result != successful) {
    result.firmwareSystemName = "Famicom";
    result.firmwareType = firmware[0].type;
    result.firmwareRegion = firmware[0].region;
    result.result = noFirmware;
    return result;
  }

  system = mia::System::create("Famicom");
  result = system->load();
  if(result != successful) return result;

  if(!ares::Famicom::load(root, "[Nintendo] Famicom (NTSC-J)")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->scan<ares::Node::Port>("Disk Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto node = root->scan<ares::Node::Setting::String>("State")) {
    node->setValue("Disk 1: Side A");
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Gamepad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return successful;
}

auto FamicomDiskSystem::unload() -> void {
  Emulator::unload();
}

auto FamicomDiskSystem::save() -> bool {
  root->save();
  system->save(system->location);
  bios->save(bios->location);
  game->save(game->location);
  return true;
}

auto FamicomDiskSystem::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Famicom") return system->pak;
  if(node->name() == "Famicom Cartridge") return bios->pak;
  if(node->name() == "Famicom Disk System") return game->pak;
  return {};
}

auto FamicomDiskSystem::notify(const string& message) -> void {
  if(auto node = root->scan<ares::Node::Setting::String>("State")) {
    node->setValue(message);
  }
}
