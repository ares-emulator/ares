struct SuperGrafx : PCEngine {
  auto name() -> string override { return "SuperGrafx"; }
  auto extensions() -> vector<string> override { return {"sgx"}; }
};
