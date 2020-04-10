#if defined(CORE_GBA)

namespace ares::GameBoyAdvance {

extern Interface* interface;

struct GameBoyAdvanceInterface : Interface {
  auto name() -> string override { return "Game Boy Advance"; }
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

struct GameBoyPlayerInterface : GameBoyAdvanceInterface {
  auto name() -> string override { return "Game Boy Player"; }
};

}

#endif
