struct SuperFamicom : System {
  auto name() -> string override { return "Super Famicom"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto SuperFamicom::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("ipl.rom", Resource::SuperFamicom::IPLROM);
  pak->append("boards.bml", Resource::SuperFamicom::Boards);
  return true;
}

auto SuperFamicom::save(string location) -> bool {
  return true;
}
