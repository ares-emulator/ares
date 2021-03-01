struct MegaCD : CompactDisc {
  auto name() -> string override { return "Mega CD"; }
  auto extensions() -> vector<string> override { return {"mcd", "cue"}; }
  auto load(string location) -> shared_pointer<vfs::directory> override;
  auto manifest(string location) -> string override;
};
