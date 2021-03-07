struct GameBoyColor : GameBoy {
  auto name() -> string override { return "Game Boy Color"; }
  auto extensions() -> vector<string> override { return {"gbc"}; }
};
