#if defined(CORE_MSX)

namespace ares::MSX {

extern Interface* interface;

struct AbstractInterface : Interface {
  auto game() -> string override;

  auto root() -> Node::Object override;
  auto load(Node::Object&) -> void override;
  auto unload() -> void override;
  auto save() -> void override;
  auto power() -> void override;
  auto run() -> void override;

  auto serialize(bool synchronize) -> serializer override;
  auto unserialize(serializer&) -> bool override;
};

struct MSXInterface : AbstractInterface {
  auto name() -> string override { return "MSX"; }
};

struct MSX2Interface : AbstractInterface {
  auto name() -> string override { return "MSX2"; }
};

}

#endif
