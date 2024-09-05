struct Pencil2 : System {
  auto name() -> string override { return "Pencil 2"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto Pencil2::load(string location) -> bool {
  auto bios = Pak::read(location);
  if(!bios) return false;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("mt.u4", bios);
  return true;
}

auto Pencil2::save(string location) -> bool {
  return true;
}
