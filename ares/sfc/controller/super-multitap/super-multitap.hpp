struct SuperMultitap : Controller {
  ControllerPort port1;
  ControllerPort port2;
  ControllerPort port3;
  ControllerPort port4;

  SuperMultitap(Node::Port);

  auto data() -> uint2;
  auto latch(bool data) -> void;

private:
  bool latched = false;
};
