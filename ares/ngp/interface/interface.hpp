#if defined(CORE_NGP)

namespace ares::NeoGeoPocket {

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

struct NeoGeoPocketInterface : AbstractInterface {
  auto name() -> string override { return "Neo Geo Pocket"; }
};

struct NeoGeoPocketColorInterface : AbstractInterface {
  auto name() -> string override { return "Neo Geo Pocket Color"; }
};

}

#endif
