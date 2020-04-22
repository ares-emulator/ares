struct PocketChallengeV2 : WonderSwan {
  auto name() -> string override { return "Pocket Challenge V2"; }
  auto extensions() -> vector<string> override { return {"pcv2", "pc2"}; }
};
