#if defined(CORE_CV)

namespace ares::ColecoVision {

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

struct ColecoVisionInterface : AbstractInterface {
  auto name() -> string override { return "ColecoVision"; }
};

//not emulated
struct ColecoAdamInterface : AbstractInterface {
  auto name() -> string override { return "ColecoAdam"; }
};

}

#endif
