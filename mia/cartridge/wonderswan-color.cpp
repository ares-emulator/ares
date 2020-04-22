struct WonderSwanColor : WonderSwan {
  auto name() -> string override { return "WonderSwan Color"; }
  auto extensions() -> vector<string> override { return {"wsc"}; }
};
