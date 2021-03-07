struct NeoGeoPocketColor : NeoGeoPocket {
  auto name() -> string override { return "Neo Geo Pocket Color"; }
  auto extensions() -> vector<string> override { return {"ngpc", "ngc"}; }
};
