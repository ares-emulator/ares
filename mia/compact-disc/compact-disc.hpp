struct CompactDisc : Media {
  auto type() -> string override { return "Compact Disc"; }
  auto extensions() -> vector<string> override { return {"cue"}; }
  auto save(string location, shared_pointer<vfs::directory> pak) -> bool override { return false; }
  auto construct() -> void override;
  auto manifest(string location) -> string override;
  auto readDataSectorBCD(string filename, u32 sectorID) -> vector<u8>;
  auto readDataSectorCUE(string filename, u32 sectorID) -> vector<u8>;
};
