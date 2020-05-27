struct FamicomDisk : FloppyDisk {
  auto name() -> string override { return "Famicom Disk"; }
  auto extensions() -> vector<string> override { return {"fds"}; }
  auto export(string location) -> vector<uint8_t> override;
  auto heuristics(vector<uint8_t>& data, string location) -> string override;
  auto import(string location) -> string override;
  auto transform(array_view<uint8_t> input) -> vector<uint8_t>;
};
