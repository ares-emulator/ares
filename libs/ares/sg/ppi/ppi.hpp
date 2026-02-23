struct PPI : I8255 {
  Node::Object node;

  //ppi.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto readPortA() -> n8 override;
  auto readPortB() -> n8 override;
  auto readPortC() -> n8 override;

  auto writePortC(n8 data) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct {
    n3 inputSelect;
  } io;
};

extern PPI ppi;
