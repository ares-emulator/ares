struct FamicomDisk : FloppyDisk {
  auto name() -> string override { return "Famicom Disk"; }
  auto extensions() -> vector<string> override { return {"fds"}; }
  auto export(string location) -> vector<u8> override;
  auto heuristics(vector<u8>& data, string location) -> string override;
  auto import(string location) -> string override;
  auto transform(array_view<u8> input) -> vector<u8>;
};
