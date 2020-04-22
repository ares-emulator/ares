struct CompactDisc : Media {
  auto type() -> string override { return "Compact Disc"; }
  auto extensions() -> vector<string> override { return {"cue"}; }
  auto construct() -> void override;
  auto manifest(string location) -> string override;
  auto import(string filename) -> string override;

  virtual auto manifest(vector<uint8_t> sector, string location) -> string = 0;
  auto readFirstDataSectorBCD(string filename) -> vector<uint8_t>;
  auto readFirstDataSectorCUE(string filename) -> vector<uint8_t>;
};
