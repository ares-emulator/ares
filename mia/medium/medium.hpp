struct Database {
  string name;
  Markup::Node list;
};

struct Medium : Pak {
  static auto create(string name) -> shared_pointer<Pak>;
  auto manifestDatabase(string sha256) -> string;

  string sha256;
};

struct Cartridge : Medium {
  auto type() -> string override { return "Cartridge"; }
};

struct CompactDisc : Medium {
  auto type() -> string override { return "Compact Disc"; }
  auto extensions() -> vector<string> override { return {"cue"}; }
  auto manifestAudio(string location) -> string;
  auto readDataSectorBCD(string filename, u32 sectorID) -> vector<u8>;
  auto readDataSectorCUE(string filename, u32 sectorID) -> vector<u8>;
};

struct FloppyDisk : Medium {
  auto type() -> string override { return "Floppy Disk"; }
};
