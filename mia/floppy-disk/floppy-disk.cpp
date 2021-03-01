#include "famicom-disk.cpp"
#include "nintendo-64dd.cpp"

auto FloppyDisk::construct() -> void {
  Media::construct();
}

auto FloppyDisk::manifest(vector<u8>& data, string location) -> string {
  string digest = Hash::SHA256(data).digest();
  for(auto game : database.find("game")) {
    if(game["sha256"].text() == digest) return BML::serialize(game);
  }
  return heuristics(data, location);
}
