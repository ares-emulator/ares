struct PlayStation : CompactDisc {
  auto name() -> string override { return "PlayStation"; }
  auto extensions() -> vector<string> override { return {"ps1", "cue", "exe"}; }
  auto pak(string location) -> shared_pointer<vfs::directory> override;
  auto rom(string location) -> vector<u8> override;
  auto manifest(string location) -> string override;
  auto cdFromExecutable(string location) -> vector<u8>;
};
