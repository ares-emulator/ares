struct SC3000 : SG1000 {
  auto name() -> string override { return "SC-3000"; }
  auto extensions() -> vector<string> override { return {"sc3000", "sc"}; }
};
