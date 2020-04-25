#if defined(CORE_PCE)

namespace ares::PCEngine {

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

struct PCEngineInterface : AbstractInterface {
  auto name() -> string override { return "PC Engine"; }
};

struct PCEngineDuoInterface : AbstractInterface {
  auto name() -> string override { return "PC Engine Duo"; }
};

struct SuperGrafxInterface : AbstractInterface {
  auto name() -> string override { return "SuperGrafx"; }
};

}

#endif
