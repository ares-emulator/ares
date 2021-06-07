struct MegaCD32X : System {
  auto name() -> string override { return "Mega CD 32X"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;

  static constexpr u8 bram[64] = {
    0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x5f,0x00,0x00,0x00,0x00,0x40,
    0x00,0x7d,0x00,0x7d,0x00,0x7d,0x00,0x7d,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x53,0x45,0x47,0x41,0x5f,0x43,0x44,0x5f,0x52,0x4f,0x4d,0x00,0x01,0x00,0x00,0x00,
    0x52,0x41,0x4d,0x5f,0x43,0x41,0x52,0x54,0x52,0x49,0x44,0x47,0x45,0x5f,0x5f,0x5f,
  };
};

auto MegaCD32X::load(string location) -> bool {
  auto bios = Pak::read(location);
  if(!bios) return false;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("tmss.rom", Resource::MegaDrive::TMSS);
  pak->append("bios.rom", bios);
  pak->append("vector.rom", Resource::Mega32X::Vector);
  pak->append("sh2.boot.mrom", Resource::Mega32X::SH2BootM);
  pak->append("sh2.boot.srom", Resource::Mega32X::SH2BootS);
  pak->append("backup.ram", 8_KiB);

  if(auto fp = pak->write("backup.ram")) {
    for(auto address : range(fp->size())) fp->write(0xff);
    fp->seek(fp->size() - sizeof(bram));
    for(auto& byte : bram) fp->write(byte);
  }

  Pak::load("backup.ram", ".bram");

  return true;
}

auto MegaCD32X::save(string location) -> bool {
  Pak::save("backup.ram", ".bram");

  return true;
}
