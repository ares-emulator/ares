struct ZXSpectrum128 : ZXSpectrum {
  ZXSpectrum128();
  auto load() -> LoadResult override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

ZXSpectrum128::ZXSpectrum128() {
  manufacturer = "Sinclair";
  name = "ZX Spectrum 128";
}

auto ZXSpectrum128::load() -> LoadResult {
  game = mia::Medium::create("ZX Spectrum");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("ZX Spectrum 128");
  result = system->load();
  if(result != successful) return result;

  if(!ares::ZXSpectrum::load(root, "[Sinclair] ZX Spectrum 128")) return otherError;

  if(auto port = root->find<ares::Node::Port>("Tape Deck/Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Keyboard")) {
    port->allocate("Original");
    port->connect();
  }

  return successful;
}

auto ZXSpectrum128::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  print(string{node->name(), "\n"});
  if(node->name() == "ZX Spectrum 128") return system->pak;
  if(node->name() == "ZX Spectrum Tape") return game->pak;
  return {};
}
