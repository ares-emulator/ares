struct MdControlPad : Controller {
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button a;
  Node::Input::Button b;
  Node::Input::Button c;
  Node::Input::Button start;

  MdControlPad(Node::Port);

  auto read() -> n7 override;
  auto write(n4 data) -> void override;

private:
  n1 yHold;
  n1 upLatch;
  n1 downLatch;
  n1 xHold;
  n1 leftLatch;
  n1 rightLatch;
  n1 th;
};
