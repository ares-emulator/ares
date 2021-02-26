struct Cartridge : Media {
  auto type() -> string override { return "Cartridge"; }
  auto construct() -> void override;
  auto manifest(string location) -> string override;
  auto manifest(vector<u8>& data, string location) -> string;
  virtual auto heuristics(vector<u8>& data, string location) -> string = 0;
};
