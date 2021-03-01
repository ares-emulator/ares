struct PlayStation : CompactDisc {
  auto name() -> string override { return "PlayStation"; }
  auto extensions() -> vector<string> override { return {"ps1", "cue", "exe"}; }
  auto load(string location) -> shared_pointer<vfs::directory> override;
  auto manifest(string location) -> string override;
  auto cdFromExecutable(string location) -> vector<u8>;
};
