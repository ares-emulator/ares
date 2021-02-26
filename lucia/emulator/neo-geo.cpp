namespace ares::NeoGeo {
  auto load(Node::System& node, string name) -> bool;
}

struct NeoGeoAES : Emulator {
  NeoGeoAES();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode, bool required) -> shared_pointer<vfs::file> override;
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

auto NeoGeoAES::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(node->name() == "Neo Geo") {
    if(auto fp = pak->find(name)) return fp;
  }
  return {};
}

auto NeoGeoAES::input(ares::Node::Input::Input node) -> void {
}

struct NeoGeoMVS : Emulator {
  NeoGeoMVS();
  auto load() -> bool override;
  auto open(ares::Node::Object, string name, vfs::file::mode, bool required) -> shared_pointer<vfs::file> override;
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

auto NeoGeoMVS::open(ares::Node::Object node, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> {
  if(node->name() == "Neo Geo") {
    if(auto fp = pak->find(name)) return fp;
  }
  return {};
}

auto NeoGeoMVS::input(ares::Node::Input::Input node) -> void {
}
