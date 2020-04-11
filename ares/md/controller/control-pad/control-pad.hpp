struct ControlPad : Controller {
  Node::Button up;
  Node::Button down;
  Node::Button left;
  Node::Button right;
  Node::Button a;
  Node::Button b;
  Node::Button c;
  Node::Button start;

  ControlPad(Node::Port);

  auto readData() -> uint8 override;
  auto writeData(uint8 data) -> void override;

private:
  uint1 select = 1;
  uint1 latch;

  bool yHold = 0;
  bool upLatch = 0;
  bool downLatch = 0;
  bool xHold = 0;
  bool leftLatch = 0;
  bool rightLatch = 0;
};
