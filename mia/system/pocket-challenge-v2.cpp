struct PocketChallengeV2 : System {
  auto name() -> string override { return "Pocket Challenge V2"; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
};

auto PocketChallengeV2::load(string location) -> LoadResult {
  this->location = locate();
  pak = std::make_shared<vfs::directory>();
  pak->append("boot.rom", Resource::PocketChallengeV2::Boot);

  return successful;
}

auto PocketChallengeV2::save(string location) -> bool {
  return true;
}
