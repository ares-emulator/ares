struct System : Pak {
  static auto create(string name) -> shared_pointer<Pak>;

  auto type() -> string override { return "System"; }
  auto extensions() -> vector<string> override { return {"sys"}; }
  auto locate() -> string;
};
