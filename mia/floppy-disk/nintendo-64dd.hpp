struct Nintendo64DD : FloppyDisk {
  auto name() -> string override { return "Nintendo 64DD"; }
  auto extensions() -> vector<string> override { return {"n64dd", "ndd"}; }
  auto pak(string location) -> shared_pointer<vfs::directory> override;
  auto rom(string location) -> vector<u8> override;
  auto heuristics(vector<u8>& data, string location) -> string override;
};
