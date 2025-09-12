struct SuperGrafx : PCEngine {
  auto name() -> string override { return "SuperGrafx"; }
  auto extensions() -> std::vector<string> override { return {"sgx"}; }
};
