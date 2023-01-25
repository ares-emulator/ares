struct NeoGeoMVS : System {
  auto name() -> string override { return "Neo Geo MVS"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto endianSwap(vector<u8>&) -> void;
};

auto NeoGeoMVS::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;

  if(location.iendsWith(".zip")) {
    Decode::ZIP archive;
    if(archive.open(location)) {
      for(auto& file : archive.file) {
        if(file.name == "sp-45.sp1") {
          auto memory = archive.extract(file);
          endianSwap(memory);
          pak->append("bios.rom", memory);
        }
      }
    }
  } else {
    auto bios = Pak::read(location);
    endianSwap(bios);
    if(bios) pak->append("bios.rom", bios);
  }

  if(pak->count() != 1) return false;

  return true;
}

auto NeoGeoMVS::save(string location) -> bool {
  return true;
}

auto NeoGeoMVS::endianSwap(vector<u8>& memory) -> void {
  for(u32 offset = 0; offset < memory.size(); offset += 2) {
    swap(memory[offset + 0], memory[offset + 1]);
  }
}
