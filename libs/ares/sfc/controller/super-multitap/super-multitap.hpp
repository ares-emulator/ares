struct SuperMultitap : Controller {
  ControllerPort port1;
  ControllerPort port2;
  ControllerPort port3;
  ControllerPort port4;

  SuperMultitap(Node::Port);

  auto data() -> n2;
  auto latch(n1 data) -> void;

private:
  n1 latched;
};
