#if defined(CORE_SG)

namespace ares::SG1000 {

extern Interface* interface;

struct AbstractInterface : Interface {
  auto game() -> string override;

  auto root() -> Node::Object override;
  auto load(Node::Object&) -> void override;
  auto power() -> void override;
  auto run() -> void override;
  auto save() -> void override;
  auto unload() -> void override;

  auto serialize(bool synchronize) -> serializer override;
  auto unserialize(serializer&) -> bool override;
};

struct SG1000Interface : AbstractInterface {
  auto name() -> string override { return "SG-1000"; }
};

struct SC3000Interface : AbstractInterface {
  auto name() -> string override { return "SC-3000"; }
};

}

#endif
