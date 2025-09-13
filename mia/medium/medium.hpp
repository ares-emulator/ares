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

struct LaserDisc : Medium {
  auto type() -> string override { return "LaserDisc"; }
  auto extensions() -> vector<string> override { return {"mmi"}; }
  auto readDataSector(string mmiPath, string cuePath, u32 sectorID) -> vector<u8>;
protected:
  Decode::MMI mmiArchive;
};

struct Tape : Medium {
  auto name() -> string override { return "Tape"; }
  auto type() -> string override { return "Tape"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  // TODO: add support for more tape formats
  auto extensions() -> vector<string> override { return {"wav"}; }
  auto analyze(string location) -> string;
};
