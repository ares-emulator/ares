struct PCEngineCD : CompactDisc {
  auto name() -> string override { return "PC Engine CD"; }
  auto extensions() -> vector<string> override { return {"pcecd", "cue"}; }
  auto load(string location) -> shared_pointer<vfs::directory> override;
  auto save(string location, shared_pointer<vfs::directory> pak) -> bool override;
  auto manifest(string location) -> string override;
};
