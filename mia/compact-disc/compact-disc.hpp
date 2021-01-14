struct CompactDisc : Media {
  auto type() -> string override { return "Compact Disc"; }
  auto extensions() -> vector<string> override { return {"cue"}; }
  auto construct() -> void override;
  auto manifest(string location) -> string override;
  auto import(string filename) -> string override;

  virtual auto manifestSector() const -> u32 = 0;
  virtual auto manifest(vector<u8> sector, string location) -> string = 0;
  auto readDataSectorBCD(string filename, u32 sectorID) -> vector<u8>;
  auto readDataSectorCUE(string filename, u32 sectorID) -> vector<u8>;
};
