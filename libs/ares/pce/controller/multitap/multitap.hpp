struct Multitap : Controller {
  ControllerPort port1;
  ControllerPort port2;
  ControllerPort port3;
  ControllerPort port4;
  ControllerPort port5;

  Multitap(Node::Port);

  auto read() -> n4 override;
  auto write(n2 data) -> void override;

private:
  u8 counter;
  u1 sel;
  u1 clr;
};
