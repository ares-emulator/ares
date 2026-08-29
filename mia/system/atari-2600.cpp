struct Atari2600 : System {
  auto name() -> string override { return "Atari 2600"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto Atari2600::load(string location) -> LoadResult {
  this->location = locate();
  pak = std::make_shared<vfs::directory>();

  static constexpr std::array<const char*, 7> KidVidFiles = {
    "KVSHARED.WAV", "KVS1.WAV", "KVS2.WAV", "KVS3.WAV",
    "KVB1.WAV", "KVB2.WAV", "KVB3.WAV",
  };
  static constexpr u64 KidVidMaximumFileSize = 64_MiB;
  auto loadKidVidFile = [&](string name, std::vector<u8> memory) {
    if(memory.empty() || pak->read(name)) return;
    pak->append(name, memory);
  };
  for(auto name : KidVidFiles) {
    auto location = string{this->location, name};
    if(file::size(location) <= KidVidMaximumFileSize) loadKidVidFile(name, file::read(location));
  }

  Decode::ZIP kidVidArchive;
  if(kidVidArchive.open({this->location, "Kid Vid Voice Module (USA) (Audio Tapes).zip"})) {
    for(auto name : KidVidFiles) {
      for(auto& item : kidVidArchive.file) {
        if(!item.name.imatch(name)) continue;
        if(item.size > KidVidMaximumFileSize) break;
        loadKidVidFile(name, kidVidArchive.extract(item));
        break;
      }
    }
  }
  return successful;
}

auto Atari2600::save(string location) -> bool {
  return true;
}
