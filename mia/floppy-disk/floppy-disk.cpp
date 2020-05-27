#include "famicom-disk.cpp"
#include "nintendo-64dd.cpp"

auto FloppyDisk::construct() -> void {
  Media::construct();
}

auto FloppyDisk::append(vector<uint8_t>& output, string filename) -> bool {
  if(!file::exists(filename)) return false;
  auto input = file::read(filename);
  auto size = output.size();
  output.resize(size + input.size());
  memory::copy(output.data() + size, input.data(), input.size());
  return true;
}

auto FloppyDisk::manifest(string location) -> string {
  vector<uint8_t> data;
  if(directory::exists(location)) {
    data = export(location);
  } else if(file::exists(location)) {
    data = file::read(location);
  }
  return manifest(data, location);
}

auto FloppyDisk::manifest(vector<uint8_t>& data, string location) -> string {
  string digest = Hash::SHA256(data).digest();
  for(auto game : database.find("game")) {
    if(game["sha256"].text() == digest) return BML::serialize(game);
  }
  return heuristics(data, location);
}
