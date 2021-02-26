struct MegaCD : CompactDisc {
  auto name() -> string override { return "Mega CD"; }
  auto extensions() -> vector<string> override { return {"mcd", "cue"}; }
  auto pak(string location) -> shared_pointer<vfs::directory> override;
  auto rom(string location) -> vector<u8> override;
  auto manifest(string location) -> string override;
};
