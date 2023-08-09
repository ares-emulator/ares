struct NeoGeoCD : System {
  auto name() -> string override { return "Neo Geo CD"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto endianSwap(vector<u8>&) -> void;
};

auto NeoGeoCD::load(string location) -> LoadResult {
  this->location = locate();
  pak = new vfs::directory;

  auto bios = Pak::read(location);
  if(!bios) return romNotFound;

  endianSwap(bios);
  pak->append("bios.rom", bios);
  pak->append("backup.ram", 8_KiB);

  if(auto fp = pak->write("backup.ram")) {
    for(auto address : range(fp->size())) fp->write(0xff);
  }

  Pak::load("backup.ram", ".bram");

  return successful;
}

auto NeoGeoCD::save(string location) -> bool {
  Pak::save("backup.ram", ".bram");

  return true;
}

auto NeoGeoCD::endianSwap(vector<u8>& memory) -> void {
  for(u32 offset = 0; offset < memory.size(); offset += 2) {
    swap(memory[offset + 0], memory[offset + 1]);
  }
}
