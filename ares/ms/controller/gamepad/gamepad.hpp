struct Gamepad : Controller {
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button one;
  Node::Input::Button two;

  Gamepad(Node::Port);

  auto read() -> n7 override;

private:
  n1 yHold;
  n1 upLatch;
  n1 downLatch;
  n1 xHold;
  n1 leftLatch;
  n1 rightLatch;
};
