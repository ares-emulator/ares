struct FamicomDisk : FloppyDisk {
  auto name() -> string override { return "Famicom Disk"; }
  auto extensions() -> vector<string> override { return {"fds"}; }
  auto load(string location) -> shared_pointer<vfs::directory> override;
  auto save(string location, shared_pointer<vfs::directory> pak) -> bool override;
  auto heuristics(vector<u8>& data, string location) -> string override;
  auto transform(array_view<u8> input) -> vector<u8>;
};
