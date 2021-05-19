struct NeoGeoAES : System {
  auto name() -> string override { return "Neo Geo AES"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto endianSwap(vector<u8>&) -> void;
};

auto NeoGeoAES::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;

  if(location.iendsWith(".zip")) {
    Decode::ZIP archive;
    if(archive.open(location)) {
      for(auto& file : archive.file) {
        if(file.name == "neo-po.bin") {
          auto memory = archive.extract(file);
          endianSwap(memory);
          pak->append("bios.rom", memory);
        }
      }
    }
  }

  return true;
}

auto NeoGeoAES::save(string location) -> bool {
  return true;
}

auto NeoGeoAES::endianSwap(vector<u8>& memory) -> void {
  for(u32 offset = 0; offset < memory.size(); offset += 2) {
    swap(memory[offset + 0], memory[offset + 1]);
  }
}
