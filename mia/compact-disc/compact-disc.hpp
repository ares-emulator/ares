struct CompactDisc : Media {
  auto type() -> string override { return "Compact Disc"; }
  auto extensions() -> vector<string> override { return {"cue"}; }
  auto construct() -> void override;
  auto manifest(string location) -> string override;
  auto import(string filename) -> string override;

  virtual auto manifestSector() const -> uint = 0;
  virtual auto manifest(vector<uint8_t> sector, string location) -> string = 0;
  auto readDataSectorBCD(string filename, uint sectorID) -> vector<uint8_t>;
  auto readDataSectorCUE(string filename, uint sectorID) -> vector<uint8_t>;
};
