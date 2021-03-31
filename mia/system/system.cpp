namespace Systems {
  #include "colecovision.cpp"
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
  #include "msx.cpp"
  #include "msx2.cpp"
  #include "neo-geo-aes.cpp"
  #include "neo-geo-mvs.cpp"
  #include "neo-geo-pocket.cpp"
  #include "neo-geo-pocket-color.cpp"
  #include "nintendo-64.cpp"
  #include "pc-engine.cpp"
  #include "saturn.cpp"
  #include "supergrafx.cpp"
  #include "playstation.cpp"
  #include "sg-1000.cpp"
  #include "sc-3000.cpp"
  #include "super-famicom.cpp"
  #include "wonderswan.cpp"
  #include "wonderswan-color.cpp"
  #include "pocket-challenge-v2.cpp"
}

auto System::create(string name) -> shared_pointer<Pak> {
  if(name == "ColecoVision") return new Systems::ColecoVision;
  if(name == "Famicom") return new Systems::Famicom;
  if(name == "Game Boy") return new Systems::GameBoy;
  if(name == "Game Boy Color") return new Systems::GameBoyColor;
  if(name == "Game Boy Advance") return new Systems::GameBoyAdvance;
  if(name == "Master System") return new Systems::MasterSystem;
  if(name == "Game Gear") return new Systems::GameGear;
  if(name == "Mega Drive") return new Systems::MegaDrive;
  if(name == "Mega 32X") return new Systems::Mega32X;
  if(name == "Mega CD") return new Systems::MegaCD;
  if(name == "Mega CD 32X") return new Systems::MegaCD32X;
  if(name == "MSX") return new Systems::MSX;
  if(name == "MSX2") return new Systems::MSX2;
  if(name == "Neo Geo AES") return new Systems::NeoGeoAES;
  if(name == "Neo Geo MVS") return new Systems::NeoGeoMVS;
  if(name == "Neo Geo Pocket") return new Systems::NeoGeoPocket;
  if(name == "Neo Geo Pocket Color") return new Systems::NeoGeoPocketColor;
  if(name == "Nintendo 64") return new Systems::Nintendo64;
  if(name == "PC Engine") return new Systems::PCEngine;
  if(name == "Saturn") return new Systems::Saturn;
  if(name == "SuperGrafx") return new Systems::SuperGrafx;
  if(name == "PlayStation") return new Systems::PlayStation;
  if(name == "SG-1000") return new Systems::SG1000;
  if(name == "SC-3000") return new Systems::SC3000;
  if(name == "Super Famicom") return new Systems::SuperFamicom;
  if(name == "WonderSwan") return new Systems::WonderSwan;
  if(name == "WonderSwan Color") return new Systems::WonderSwanColor;
  if(name == "Pocket Challenge V2") return new Systems::PocketChallengeV2;
  return {};
}

auto System::locate() -> string {
  string location = {mia::homeLocation(), name(), ".sys/"};
  directory::create(location);
  return location;
}
