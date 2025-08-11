struct Database {
  string name;
  Markup::Node list;
};

struct Medium : Pak {
  static auto create(string name) -> shared_pointer<Pak>;
  auto loadDatabase() -> bool;
  auto database() -> Database;
  auto manifestDatabase(string sha256) -> string;
  auto manifestDatabaseArcade(string name) -> string;

  string sha256;
};

struct Cartridge : Medium {
  auto type() -> string override { return "Cartridge"; }
};

struct CompactDisc : Medium {
  auto type() -> string override { return "Compact Disc"; }
  auto extensions() -> vector<string> override {
#if defined(ARES_ENABLE_CHD)
    return {"cue", "chd"};
#else
    return {"cue"};
#endif
  }
  auto isAudioCd(string location) -> bool;
  auto manifestAudio(string location) -> string;
  auto readDataSector(string filename, u32 sectorID) -> vector<u8>;
  auto readDataSectorMMI(string filename, string containedFilePath, u32 sectorID) -> vector<u8>;
private:
  auto readDataSectorBCD(string filename, u32 sectorID) -> vector<u8>;
  auto readDataSectorCUE(string filename, u32 sectorID) -> vector<u8>;
#if defined(ARES_ENABLE_CHD)
  auto readDataSectorCHD(string filename, u32 sectorID) -> vector<u8>;
#endif
};

struct FloppyDisk : Medium {
  auto type() -> string override { return "Floppy Disk"; }
};
