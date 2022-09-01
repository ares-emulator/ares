struct Nintendo64DD : System {
  auto name() -> string override { return "Nintendo 64DD"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto Nintendo64DD::load(string location) -> bool {
  auto bios = Pak::read(location);
  if(!bios) return false;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("pif.ntsc.rom", Resource::Nintendo64::PIFNTSC);
  pak->append("pif.pal.rom",  Resource::Nintendo64::PIFPAL );
  pak->append("pif.sm5.rom",  Resource::Nintendo64::PIFSM5 );
  pak->append("64dd.ipl.rom", bios);
  return true;
}

auto Nintendo64DD::save(string location) -> bool {
  return true;
}
