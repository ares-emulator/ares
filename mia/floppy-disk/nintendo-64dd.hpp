struct Nintendo64DD : FloppyDisk {
  auto name() -> string override { return "Nintendo 64DD"; }
  auto extensions() -> vector<string> override { return {"n64dd", "ndd"}; }
  auto load(string location) -> shared_pointer<vfs::directory> override;
  auto save(string location, shared_pointer<vfs::directory> pak) -> bool override;
  auto heuristics(vector<u8>& data, string location) -> string override;
};
