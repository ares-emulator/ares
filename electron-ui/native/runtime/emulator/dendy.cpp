struct Dendy : Famicom {
  Dendy();
  auto load() -> LoadResult override;
};

Dendy::Dendy() {
  manufacturer = "Dendy";
  name = "Dendy";

  ports.clear();
  allocatePorts(false);
}

auto Dendy::load() -> LoadResult {
  game = mia::Medium::create("Famicom");
  string location = Emulator::load(game, configuration.game);
  if(!location) return noFileSelected;
  LoadResult result = game->load(location);
  if(result != successful) return result;

  system = mia::System::create("Famicom");
  result = system->load();
  if(result != successful) return result;

  if(!ares::Famicom::load(root, "[Dendy] Dendy")) return otherError;

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

  return successful;
}
