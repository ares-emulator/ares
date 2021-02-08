struct Gamepad : Controller {
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button a;
  Node::Input::Button b;

  Gamepad(Node::Port);

  auto read() -> n6 override;

private:
  b1 yHold;
  b1 upLatch;
  b1 downLatch;
  b1 xHold;
  b1 leftLatch;
  b1 rightLatch;
};
