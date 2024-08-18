struct PocketChallengeV2 : WonderSwan {
  auto name() -> string override { return "Pocket Challenge V2"; }
  auto extensions() -> vector<string> override { return {"pcv2", "pc2"}; }
  auto mapper(vector<u8>& rom) -> string override;
};

auto PocketChallengeV2::mapper(vector<u8>& rom) -> string {
  return "KARNAK";
}
