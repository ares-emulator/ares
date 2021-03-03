struct NeoGeoMVS : Emulator {
  NeoGeoMVS();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

NeoGeoMVS::NeoGeoMVS() {
  medium = mia::medium("Neo Geo");
  manufacturer = "SNK";
  name = "Neo Geo MVS";
}

auto NeoGeoMVS::load() -> bool {
  if(!ares::NeoGeo::load(root, "[SNK] Neo Geo MVS")) return false;

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

auto NeoGeoMVS::save() -> bool {
  root->save();
  medium->save(game.location, game.pak);
  return true;
}

auto NeoGeoMVS::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Neo Geo Cartridge") return game.pak;
  return {};
}

auto NeoGeoMVS::input(ares::Node::Input::Input node) -> void {
}
