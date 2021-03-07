struct MSX2 : MSX {
  auto name() -> string override { return "MSX2"; }
  auto extensions() -> vector<string> override { return {"msx2"}; }
};
