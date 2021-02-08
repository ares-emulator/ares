struct Gamepad : Controller {
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button two;
  Node::Input::Button one;
  Node::Input::Button select;
  Node::Input::Button run;

  Gamepad(Node::Port);

  auto read() -> n4 override;
  auto write(n2 data) -> void override;

private:
  b1 sel;
  b1 clr;

  b1 yHold;
  b1 upLatch;
  b1 downLatch;
  b1 xHold;
  b1 leftLatch;
  b1 rightLatch;
};
