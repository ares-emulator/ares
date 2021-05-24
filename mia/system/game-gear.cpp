struct GameGear : System {
  auto name() -> string override { return "Game Gear"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto GameGear::load(string location) -> bool {
  auto bios = Pak::read(location);  //optional

  this->location = locate();
  pak = new vfs::directory;
  if(bios) pak->append("bios.rom", bios);

  return true;
}

auto GameGear::save(string location) -> bool {
  return true;
}
