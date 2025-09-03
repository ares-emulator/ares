struct WonderSwanColor : WonderSwan {
  auto name() -> string override { return "WonderSwan Color"; }
  auto extensions() -> std::vector<string> override { return {"wsc"}; }
};
