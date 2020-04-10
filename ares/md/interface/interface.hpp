#if defined(CORE_MD)

namespace ares::MegaDrive {

extern Interface* interface;

struct MegaDriveInterface : Interface {
  auto name() -> string override { return "Mega Drive"; }
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

}

#endif
