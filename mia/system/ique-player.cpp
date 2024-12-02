struct iQuePlayer : System {
  auto name() -> string override { return "iQue Player"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto iQuePlayer::load(string location) -> bool {
  auto boot = Pak::read(location);
  if(!boot) return false;

  this->location = locate();
  pak = new vfs::directory;
  pak->append("boot.rom", boot);
  pak->append("time.rtc", 0x10);
  pak->append("nand.flash", 0);
  pak->append("spare.flash", 0);
  pak->append("virage0.flash", 0x40);
  pak->append("virage1.flash", 0x40);
  pak->append("virage2.flash", 0);

  if(auto fp = pak->write("time.rtc")) {
    for(auto address : range(fp->size())) fp->write(0xff);
  }

  if(auto fp = pak->write("virage0.flash")) {
    for(auto address : range(fp->size())) fp->write(0x00);
  }

  if(auto fp = pak->write("virage1.flash")) {
    for(auto address : range(fp->size())) fp->write(0x00);
  }

  Pak::load("time.rtc", ".rtc");
  Pak::load("nand.flash", ".flash");
  Pak::load("spare.flash", ".flash");
  Pak::load("virage0.flash", ".flash");
  Pak::load("virage1.flash", ".flash");
  Pak::load("virage2.flash", ".flash");

  return true;
}

auto iQuePlayer::save(string location) -> bool {
  Pak::save("time.rtc", ".rtc");
  Pak::save("nand.flash", ".flash");
  Pak::save("spare.flash", ".flash");
  Pak::save("virage0.flash", ".flash");
  Pak::save("virage1.flash", ".flash");
  Pak::save("virage2.flash", ".flash");

  return true;
}