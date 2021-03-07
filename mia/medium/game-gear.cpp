struct GameGear : MasterSystem {
  auto name() -> string override { return "Game Gear"; }
  auto extensions() -> vector<string> override { return {"gg"}; }
};
