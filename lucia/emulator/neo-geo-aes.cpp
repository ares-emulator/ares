struct NeoGeoAES : Emulator {
  NeoGeoAES();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

NeoGeoAES::NeoGeoAES() {
  medium = mia::medium("Neo Geo");
  manufacturer = "SNK";
  name = "Neo Geo AES";
}

auto NeoGeoAES::load() -> bool {
  if(!ares::NeoGeo::load(root, "[SNK] Neo Geo AES")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Arcade Stick");
    port->connect();
  }

  return true;
}

auto NeoGeoAES::save() -> bool {
  root->save();
  medium->save(game.location, game.pak);
  return true;
}

auto NeoGeoAES::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Neo Geo Cartridge") return game.pak;
  return {};
}

auto NeoGeoAES::input(ares::Node::Input::Input node) -> void {
}
