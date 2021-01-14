struct Nintendo64DD : FloppyDisk {
  auto name() -> string override { return "Nintendo 64DD"; }
  auto extensions() -> vector<string> override { return {"ndd"}; }
  auto export(string location) -> vector<u8> override;
  auto heuristics(vector<u8>& data, string location) -> string override;
  auto import(string location) -> string override;
};
