struct NeoGeoPocketColor : NeoGeoPocket {
  auto name() -> string override { return "Neo Geo Pocket Color"; }
  auto extensions() -> std::vector<string> override { return {"ngpc", "ngc"}; }
};
