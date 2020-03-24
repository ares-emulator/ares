struct CompactDisc : Media {
  auto type() -> string override { return "Compact Disc"; }
  auto construct() -> void override;
  auto manifest(string location) -> string override;
  auto import(string filename) -> string override;

  auto importCUE(CD::Session&, string) -> string;
};
