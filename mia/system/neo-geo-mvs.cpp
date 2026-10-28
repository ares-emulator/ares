struct NeoGeoMVS : System {
  auto name() -> string override { return "Neo Geo MVS"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto endianSwap(std::vector<u8>&) -> void;
};

auto NeoGeoMVS::load(string location) -> LoadResult {
  this->location = locate();
  pak = std::make_shared<vfs::directory>();

  if(location.iendsWith(".zip")) {
    Decode::ZIP archive;
    if(archive.open(location)) {
      for(auto& file : archive.file) {
        if(file.name == (biosName ? biosName : string{"vs-bios.rom"})) {
          auto extracted = archive.extract(file);
          std::vector<u8> memory;
          memory.resize(extracted.size());
          if(!extracted.empty()) memcpy(memory.data(), extracted.data(), extracted.size());
          endianSwap(memory);
          pak->append("bios.rom", memory);
        }
      }
    }
  } else {
    auto bios = Pak::read(location);
    endianSwap(bios);
    if(!bios.empty()) pak->append("bios.rom", bios);
  }

  if(pak->count() != 1) return romNotFound;

  pak->append("time.rtc", 0x10);
  if(auto fp = pak->write("time.rtc")) {
    for(auto address : range(fp->size())) fp->write(0xff);
  }
  Pak::load("time.rtc", ".rtc");

  auto databaseFile = mia::locate("Database/NeoGeoMVS.bml");
  if(inode::exists(databaseFile)) {
    auto database = BML::unserialize(file::read(databaseFile));
    auto games = database.find("game");
    if(games.size() == 1) pak->append("manifest.bml", BML::serialize(games.front()));
  }

  return successful;
}

auto NeoGeoMVS::save(string location) -> bool {
  Pak::save("time.rtc", ".rtc");
  return true;
}

auto NeoGeoMVS::endianSwap(std::vector<u8>& memory) -> void {
  for(u32 offset = 0; offset + 1 < memory.size(); offset += 2) {
    swap(memory[offset + 0], memory[offset + 1]);
  }
}
