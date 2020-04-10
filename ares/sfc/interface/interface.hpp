#if defined(CORE_SFC)

namespace ares::SuperFamicom {

extern Interface* interface;

struct SuperFamicomInterface : Interface {
  auto name() -> string override { return "Super Famicom"; }
  auto game() -> string override;

  auto root() -> Node::Object override;
  auto load(Node::Object&) -> void override;
  auto unload() -> void override;
  auto save() -> void override;
  auto power() -> void override;
  auto run() -> void override;

  auto serialize(bool synchronize = true) -> serializer override;
  auto unserialize(serializer&) -> bool override;

  auto exportMemory() -> bool override;
};

}

#endif
