#include "bs-memory.cpp"
#include "colecovision.cpp"
#include "famicom.cpp"
#include "game-boy.cpp"
#include "game-boy-advance.cpp"
#include "game-boy-color.cpp"
#include "master-system.cpp"
#include "mega-drive.cpp"
#include "game-gear.cpp"
#include "msx.cpp"
#include "msx2.cpp"
#include "neo-geo.cpp"
#include "neo-geo-pocket.cpp"
#include "neo-geo-pocket-color.cpp"
#include "nintendo-64.cpp"
#include "pc-engine.cpp"
#include "sg-1000.cpp"
#include "sc-3000.cpp"
#include "sufami-turbo.cpp"
#include "super-famicom.cpp"
#include "supergrafx.cpp"
#include "wonderswan.cpp"
#include "wonderswan-color.cpp"
#include "pocket-challenge-v2.cpp"

auto Cartridge::construct() -> void {
  Media::construct();
}



auto Cartridge::manifest(string location) -> string {
  vector<u8> data;
  if(directory::exists(location)) {
    data = rom(location);
  } else if(file::exists(location)) {
    data = file::read(location);
  }
  return manifest(data, location);
}

auto Cartridge::manifest(vector<u8>& data, string location) -> string {
  string digest = Hash::SHA256(data).digest();
  for(auto game : database.find("game")) {
    if(game["sha256"].string() == digest) return BML::serialize(game);
  }
  return heuristics(data, location);
}
