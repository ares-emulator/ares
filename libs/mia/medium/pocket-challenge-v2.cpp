struct PocketChallengeV2 : WonderSwan {
  auto name() -> string override { return "Pocket Challenge V2"; }
  auto extensions() -> std::vector<string> override { return {"pcv2", "pc2"}; }
  auto mapper(std::vector<u8>& rom) -> string override;
};

auto PocketChallengeV2::mapper(std::vector<u8>& rom) -> string {
  return "KARNAK";
}
