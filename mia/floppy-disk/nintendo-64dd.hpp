struct Nintendo64DD : FloppyDisk {
  auto name() -> string override { return "Nintendo 64DD"; }
  auto extensions() -> vector<string> override { return {"ndd"}; }
  auto export(string location) -> vector<uint8_t> override;
  auto heuristics(vector<uint8_t>& data, string location) -> string override;
  auto import(string location) -> string override;
};
