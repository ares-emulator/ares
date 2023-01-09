struct ZXSpectrum128 : ZXSpectrum {
  ZXSpectrum128();
  auto load() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

ZXSpectrum128::ZXSpectrum128() {
  manufacturer = "Sinclair";
  name = "ZX Spectrum 128";
}

auto ZXSpectrum128::load() -> bool {
  game = mia::Medium::create("ZX Spectrum");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("ZX Spectrum 128");
  if(!system->load()) return false;

  if(!ares::ZXSpectrum::load(root, "[Sinclair] ZX Spectrum 128")) return false;

  if(auto port = root->find<ares::Node::Port>("Tape Deck/Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Keyboard")) {
    port->allocate("Original");
    port->connect();
  }

  return true;
}

auto ZXSpectrum128::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  print(string{node->name(), "\n"});
  if(node->name() == "ZX Spectrum 128") return system->pak;
  if(node->name() == "ZX Spectrum Tape") return game->pak;
  return {};
}
