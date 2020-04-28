#if defined(CORE_N64)

namespace ares::Nintendo64 {

extern Interface* interface;

struct Nintendo64Interface : Interface {
  auto name() -> string override { return "Nintendo 64"; }
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
