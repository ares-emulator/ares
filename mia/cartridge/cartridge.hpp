struct Cartridge : Media {
  auto type() -> string override { return "Cartridge"; }
  auto construct() -> void override;
  auto manifest(string location) -> string override;
  auto import(string filename) -> string override;

  auto append(vector<uint8_t>& data, string filename) -> bool;
  auto manifest(vector<uint8_t>& data, string location) -> string;

  virtual auto export(string location) -> vector<uint8_t> = 0;
  virtual auto heuristics(vector<uint8_t>& data, string location) -> string = 0;
};
