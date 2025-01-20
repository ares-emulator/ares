struct NeoGeoAES : System {
  auto name() -> string override { return "Neo Geo AES"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto endianSwap(vector<u8>&) -> void;
};

auto NeoGeoAES::load(string location) -> LoadResult {
  this->location = locate();
  pak = new vfs::directory;

  if(location.iendsWith(".zip")) {
    Decode::ZIP archive;
    if(archive.open(location)) {
      for(auto& file : archive.file) {
        if(file.name == "neo-epo.bin") {
          auto memory = archive.extract(file);
          endianSwap(memory);
          pak->append("bios.rom", memory);
        }
      }
    }
  } else {
    auto bios = Pak::read(location);
    endianSwap(bios);
    if (bios) pak->append("bios.rom", bios);
  }

  if(pak->count() != 1) return LoadResult(romNotFound);

  return LoadResult(successful);
}

auto NeoGeoAES::save(string location) -> bool {
  return true;
}

auto NeoGeoAES::endianSwap(vector<u8>& memory) -> void {
  for(u32 offset = 0; offset < memory.size(); offset += 2) {
    swap(memory[offset + 0], memory[offset + 1]);
  }
}
