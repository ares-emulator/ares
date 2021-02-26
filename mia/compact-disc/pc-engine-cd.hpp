struct PCEngineCD : CompactDisc {
  auto name() -> string override { return "PC Engine CD"; }
  auto extensions() -> vector<string> override { return {"pcecd", "cue"}; }
  auto pak(string location) -> shared_pointer<vfs::directory> override;
  auto rom(string location) -> vector<u8> override;
  auto manifest(string location) -> string override;
};
