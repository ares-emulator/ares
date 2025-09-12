struct GameBoyColor : GameBoy {
  auto name() -> string override { return "Game Boy Color"; }
  auto saveName() -> string override { return "Game Boy"; }
  auto extensions() -> std::vector<string> override { return {"gb", "gbc"}; }
};
