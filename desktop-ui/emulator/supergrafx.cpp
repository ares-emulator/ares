struct SuperGrafx : PCEngine {
  SuperGrafx();
  auto load() -> LoadResult override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
};

SuperGrafx::SuperGrafx() {
  manufacturer = "NEC";
  name = "SuperGrafx";

  allocatePorts();
}

auto SuperGrafx::load() -> LoadResult {
  game = mia::Medium::create("SuperGrafx");
  string location = Emulator::load(game, configuration.game);
  if(!location) return LoadResult(noFileSelected);
  LoadResult result = game->load(location);
  if(result != LoadResult(successful)) return result;

  system = mia::System::create("SuperGrafx");
  result = system->load();
  if(result != LoadResult(successful)) return result;

  ares::PCEngine::option("Pixel Accuracy", settings.video.pixelAccuracy);

  if(!ares::PCEngine::load(root, "[NEC] SuperGrafx (NTSC-J)")) return LoadResult(otherError);

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  connectPorts();

  return LoadResult(successful);
}

auto SuperGrafx::save() -> bool {
  root->save();
  system->save(game->location);
  game->save(game->location);
  return true;
}

auto SuperGrafx::pak(ares::Node::Object node) -> shared_pointer<vfs::directory> {
  if(node->name() == "SuperGrafx") return system->pak;
  if(node->name() == "SuperGrafx Card") return game->pak;
  return {};
}
