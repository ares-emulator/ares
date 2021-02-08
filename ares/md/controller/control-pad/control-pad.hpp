struct ControlPad : Controller {
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button a;
  Node::Input::Button b;
  Node::Input::Button c;
  Node::Input::Button start;

  ControlPad(Node::Port);

  auto readData() -> n8 override;
  auto writeData(n8 data) -> void override;

private:
  n1 select = 1;
  n1 latch;

  b1 yHold;
  b1 upLatch;
  b1 downLatch;
  b1 xHold;
  b1 leftLatch;
  b1 rightLatch;
};
