namespace Systems {
  #include "arcade.cpp"
  #include "atari-2600.cpp"
  #include "colecovision.cpp"
  #include "myvision.cpp"
  #include "famicom.cpp"
  #include "game-boy.cpp"
  #include "game-boy-color.cpp"
  #include "game-boy-advance.cpp"
  #include "master-system.cpp"
  #include "game-gear.cpp"
  #include "mega-drive.cpp"
  #include "mega-32x.cpp"
  #include "mega-cd.cpp"
  #include "mega-cd-32x.cpp"
  #include "mega-ld.cpp"
  #include "msx.cpp"
  #include "msx2.cpp"
  #include "neo-geo-aes.cpp"
  #include "neo-geo-mvs.cpp"
  #include "neo-geo-pocket.cpp"
  #include "neo-geo-pocket-color.cpp"
  #include "nintendo-64.cpp"
  #include "nintendo-64dd.cpp"
  #include "pc-engine.cpp"
  #include "pc-engine-ld.cpp"
  #include "saturn.cpp"
  #include "supergrafx.cpp"
  #include "playstation.cpp"
  #include "sg-1000.cpp"
  #include "sc-3000.cpp"
  #include "super-famicom.cpp"
  #include "wonderswan.cpp"
  #include "wonderswan-color.cpp"
  #include "pocket-challenge-v2.cpp"
  #include "zx-spectrum.cpp"
  #include "zx-spectrum-128.cpp"
}

auto System::create(string name) -> std::shared_ptr<Pak> {
  if(name == "Arcade") return std::make_shared<Systems::Arcade>();
  if(name == "Atari 2600") return std::make_shared<Systems::Atari2600>();
  if(name == "ColecoVision") return std::make_shared<Systems::ColecoVision>();
  if(name == "MyVision") return std::make_shared<Systems::MyVision>();
  if(name == "Famicom") return std::make_shared<Systems::Famicom>();
  if(name == "Game Boy") return std::make_shared<Systems::GameBoy>();
  if(name == "Game Boy Color") return std::make_shared<Systems::GameBoyColor>();
  if(name == "Game Boy Advance") return std::make_shared<Systems::GameBoyAdvance>();
  if(name == "Master System") return std::make_shared<Systems::MasterSystem>();
  if(name == "Game Gear") return std::make_shared<Systems::GameGear>();
  if(name == "Mega Drive") return std::make_shared<Systems::MegaDrive>();
  if(name == "Mega 32X") return std::make_shared<Systems::Mega32X>();
  if(name == "Mega CD") return std::make_shared<Systems::MegaCD>();
  if(name == "Mega LD") return std::make_shared<Systems::MegaLD>();
  if(name == "Mega CD 32X") return std::make_shared<Systems::MegaCD32X>();
  if(name == "MSX") return std::make_shared<Systems::MSX>();
  if(name == "MSX2") return std::make_shared<Systems::MSX2>();
  if(name == "Neo Geo AES") return std::make_shared<Systems::NeoGeoAES>();
  if(name == "Neo Geo MVS") return std::make_shared<Systems::NeoGeoMVS>();
  if(name == "Neo Geo Pocket") return std::make_shared<Systems::NeoGeoPocket>();
  if(name == "Neo Geo Pocket Color") return std::make_shared<Systems::NeoGeoPocketColor>();
  if(name == "Nintendo 64") return std::make_shared<Systems::Nintendo64>();
  if(name == "Nintendo 64DD") return std::make_shared<Systems::Nintendo64DD>();
  if(name == "PC Engine") return std::make_shared<Systems::PCEngine>();
  if(name == "PC Engine LD") return std::make_shared<Systems::PCEngineLD>();
  if(name == "Saturn") return std::make_shared<Systems::Saturn>();
  if(name == "SuperGrafx") return std::make_shared<Systems::SuperGrafx>();
  if(name == "PlayStation") return std::make_shared<Systems::PlayStation>();
  if(name == "SG-1000") return std::make_shared<Systems::SG1000>();
  if(name == "SC-3000") return std::make_shared<Systems::SC3000>();
  if(name == "Super Famicom") return std::make_shared<Systems::SuperFamicom>();
  if(name == "WonderSwan") return std::make_shared<Systems::WonderSwan>();
  if(name == "WonderSwan Color") return std::make_shared<Systems::WonderSwanColor>();
  if(name == "Pocket Challenge V2") return std::make_shared<Systems::PocketChallengeV2>();
  if(name == "ZX Spectrum") return std::make_shared<Systems::ZXSpectrum>();
  if(name == "ZX Spectrum 128") return std::make_shared<Systems::ZXSpectrum128>();
  return {};
}

auto System::locate() -> string {
  string location = {mia::homeLocation(), name(), ".sys/"};
  directory::create(location);
  return location;
}
