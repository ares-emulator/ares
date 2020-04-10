#if defined(CORE_WS)

namespace ares::WonderSwan {

extern Interface* interface;

struct AbstractInterface : Interface {
  auto game() -> string override;

  auto root() -> Node::Object override;
  auto load(Node::Object&) -> void override;
  auto unload() -> void override;
  auto save() -> void override;
  auto power() -> void override;
  auto run() -> void override;

  auto serialize(bool synchronize = true) -> serializer override;
  auto unserialize(serializer&) -> bool override;
};

struct WonderSwanInterface : AbstractInterface {
  auto name() -> string override { return "WonderSwan"; }
};

struct WonderSwanColorInterface : AbstractInterface {
  auto name() -> string override { return "WonderSwan Color"; }
};

struct SwanCrystalInterface : AbstractInterface {
  auto name() -> string override { return "SwanCrystal"; }
};

struct PocketChallengeV2Interface : AbstractInterface {
  auto name() -> string override { return "Pocket Challenge V2"; }
};

struct MamaMitteInterface : AbstractInterface {
  auto name() -> string override { return "Mama Mitte"; }
};

}

#endif
