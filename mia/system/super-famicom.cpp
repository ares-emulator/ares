struct SuperFamicom : System {
  auto name() -> string override { return "Super Famicom"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto SuperFamicom::load(string location) -> LoadResult {
  this->location = locate();
  pak = std::make_shared<vfs::directory>();
  pak->append("ipl.rom", Resource::SuperFamicom::IPLROM);
  auto romLocation = mia::locate("Database/Super Famicom Boards.bml");
  if(!romLocation) return { databaseNotFound, "Super Famicom Boards.bml" };
  pak->append("boards.bml", file::read(romLocation));
  return successful;
}

auto SuperFamicom::save(string location) -> bool {
  return true;
}
