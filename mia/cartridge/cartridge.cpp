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

auto Cartridge::append(vector<uint8_t>& output, string filename) -> bool {
  if(!file::exists(filename)) return false;
  auto input = file::read(filename);
  auto size = output.size();
  output.resize(size + input.size());
  memory::copy(output.data() + size, input.data(), input.size());
  return true;
}

auto Cartridge::import(string location) -> string {
  auto data = Media::read(location);
  auto manifest = this->manifest(data, location);
  if(!manifest) return "failed to parse ROM";

  auto document = BML::unserialize(manifest);
  location = {pathname, Location::prefix(location), "/"};
  if(!directory::create(location)) return "output directory not writable";

  if(settings.createManifests) {
    file::write({location, "manifest.bml"}, manifest);
  }

  auto buffer = array_view<uint8_t>{data};
  for(auto memory : document.find("game/board/memory")) {
    auto type = memory["type"].text();
    auto size = memory["size"].natural();
    auto content = memory["content"].text();
    auto architecture = memory["architecture"].text();
    auto identifier = memory["identifier"].text();

    bool write = false;
    if(type == "ROM") write = true;
    if(type == "Flash" && content != "Save") write = true;
    if(!write) continue;

    string filename{content, ".", type};
    if(architecture) filename.prepend(architecture, ".");
    filename.downcase();

    if(size > buffer.size()) return {"missing ", filename};

    file::write({location, filename}, {buffer, size});
    buffer += size;
  }

  return {};
}

auto Cartridge::manifest(string location) -> string {
  vector<uint8_t> data;
  if(directory::exists(location)) {
    data = export(location);
  } else if(file::exists(location)) {
    data = file::read(location);
  }
  return manifest(data, location);
}

auto Cartridge::manifest(vector<uint8_t>& data, string location) -> string {
  string digest = Hash::SHA256(data).digest();
  for(auto game : database.find("game")) {
    if(game["sha256"].text() == digest) return BML::serialize(game);
  }
  return heuristics(data, location);
}
