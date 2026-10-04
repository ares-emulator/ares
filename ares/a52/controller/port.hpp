struct ControllerPort {
  Node::Port port;
  std::unique_ptr<Controller> device;

  //port.cpp
  ControllerPort(string name);

  auto load(Node::Object parent) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  auto connected() const -> bool;
  auto poll() -> void;
  auto axis(u32 index, bool powered) -> s16;
  auto keypad(n4 code) -> bool;
  auto topFire() -> bool;
  auto bottomFire() -> bool;

  const string name;
};

extern ControllerPort controllerPorts[4];
