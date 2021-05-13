struct Nintendo64 : System {
  auto name() -> string override { return "Nintendo 64"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto Nintendo64::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("pif.ntsc.rom", Resource::Nintendo64::PIFNTSC);
  pak->append("pif.pal.rom",  Resource::Nintendo64::PIFPAL );
  pak->append("pif.sm5.rom",  Resource::Nintendo64::PIFSM5 );
  return true;
}

auto Nintendo64::save(string location) -> bool {
  return true;
}
