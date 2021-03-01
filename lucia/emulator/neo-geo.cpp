namespace ares::NeoGeo {
  auto load(Node::System& node, string name) -> bool;
}

struct NeoGeoAES : Emulator {
  NeoGeoAES();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto input(ares::Node::Input::Input) -> void override;
};

struct NeoGeoMVS : Emulator {
  NeoGeoMVS();
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
  return medium->save(game.location, game.pak);
}

auto NeoGeoAES::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Neo Geo") return game.pak;
  return {};
}

auto NeoGeoAES::input(ares::Node::Input::Input node) -> void {
}

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
  return medium->save(game.location, game.pak);
}

auto NeoGeoMVS::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "Neo Geo") return game.pak;
  return {};
}

auto NeoGeoMVS::input(ares::Node::Input::Input node) -> void {
}
