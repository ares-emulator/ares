struct PlayStation : CompactDisc {
  auto name() -> string override { return "PlayStation"; }
  auto extensions() -> vector<string> override { return {"cue", "exe"}; }
  auto manifest(string location) -> string override;
  auto manifestSector() const -> u32 override { return 4; }
  auto manifest(vector<u8> sector, string location) -> string override;
  auto cdFromExecutable(string location) -> vector<u8>;
};
