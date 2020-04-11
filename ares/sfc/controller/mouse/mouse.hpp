struct Mouse : Controller {
  Node::Axis x;
  Node::Axis y;
  Node::Button left;
  Node::Button right;

  Mouse(Node::Port);

  auto data() -> uint2;
  auto latch(bool data) -> void;

private:
  bool latched = 0;
  uint counter = 0;

  uint2 speed;  //0 = slow, 1 = normal, 2 = fast
  int32 cx;     //x-coordinate
  int32 cy;     //y-coordinate
  uint1 dx;     //x-direction
  uint1 dy;     //y-direction
};
