struct SC3000 : SG1000 {
  auto name() -> string override { return "SC-3000"; }
  auto extensions() -> std::vector<string> override { return {"sg", "sc"}; }
};
