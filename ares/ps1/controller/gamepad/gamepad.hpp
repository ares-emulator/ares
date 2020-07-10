struct Gamepad : Controller {
  Node::Button up;
  Node::Button down;
  Node::Button left;
  Node::Button right;
  Node::Button cross;
  Node::Button circle;
  Node::Button square;
  Node::Button triangle;
  Node::Button l1;
  Node::Button l2;
  Node::Button r1;
  Node::Button r2;
  Node::Button select;
  Node::Button start;

  Node::Axis lx;
  Node::Axis ly;
  Node::Axis rx;
  Node::Axis ry;
  Node::Button lt;
  Node::Button rt;

  Gamepad(Node::Port);
  auto read() -> u64 override;
};
