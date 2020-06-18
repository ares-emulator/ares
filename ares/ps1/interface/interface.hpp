#if defined(CORE_PS1)

namespace ares::PlayStation {

extern Interface* interface;

struct PlayStationInterface : Interface {
  auto name() -> string override { return "PlayStation"; }
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

}

#endif
