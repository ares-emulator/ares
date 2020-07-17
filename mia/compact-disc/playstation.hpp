struct PlayStation : CompactDisc {
  auto name() -> string override { return "PlayStation"; }
  auto extensions() -> vector<string> override { return {"cue", "exe"}; }
  auto manifest(string location) -> string override;
  auto manifestSector() const -> uint override { return 4; }
  auto manifest(vector<uint8_t> sector, string location) -> string override;
  auto cdFromExecutable(string location) -> vector<uint8_t>;
};
