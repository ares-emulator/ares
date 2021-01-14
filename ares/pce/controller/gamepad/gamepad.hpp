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

  auto read() -> uint4 override;
  auto write(uint2 data) -> void override;

private:
  bool sel = 0;
  bool clr = 0;

  bool yHold = 0;
  bool upLatch = 0;
  bool downLatch = 0;
  bool xHold = 0;
  bool leftLatch = 0;
  bool rightLatch = 0;
};
