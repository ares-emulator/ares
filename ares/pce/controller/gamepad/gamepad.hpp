struct Gamepad : Controller {
  Node::Button up;
  Node::Button down;
  Node::Button left;
  Node::Button right;
  Node::Button two;
  Node::Button one;
  Node::Button select;
  Node::Button run;

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
