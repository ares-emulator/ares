struct PocketChallengeV2 : System {
  auto name() -> string override { return "Pocket Challenge V2"; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
};

auto PocketChallengeV2::load(string location) -> bool {
  this->location = locate();
  pak = new vfs::directory;
  pak->append("boot.rom", Resource::PocketChallengeV2::Boot);

  return true;
}

auto PocketChallengeV2::save(string location) -> bool {
  return true;
}
