struct AvenuePad : Controller {
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button three;
  Node::Input::Button two;
  Node::Input::Button one;
  Node::Input::Button four;
  Node::Input::Button five;
  Node::Input::Button six;
  Node::Input::Button select;
  Node::Input::Button run;

  AvenuePad(Node::Port);

  auto read() -> n4 override;
  auto write(n2 data) -> void override;

private:
  bool sel = 0;
  bool clr = 0;
  bool active = 0;

  bool yHold = 0;
  bool upLatch = 0;
  bool downLatch = 0;
  bool xHold = 0;
  bool leftLatch = 0;
  bool rightLatch = 0;
};
