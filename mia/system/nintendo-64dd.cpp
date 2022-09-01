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
  pak->append("time.rtc", 6);

  if(auto fp = pak->write("time.rtc")) {
    for(auto address : range(fp->size())) fp->write(0xff);
  }

  Pak::load("time.rtc", ".rtc");
  return true;
}

auto Nintendo64DD::save(string location) -> bool {
  Pak::save("time.rtc", ".rtc");
  
  return true;
}
