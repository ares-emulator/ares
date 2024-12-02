struct iQuePlayer : Medium {
  auto name() -> string override { return "iQue Player"; }
  auto extensions() -> vector<string> override { return {"bb"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
};

auto iQuePlayer::load(string location) -> bool {
  vector<u8> nand;
  if(directory::exists(location)) {
    append(rom, {location, ""})
  }
}