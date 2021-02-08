struct Mouse : Controller {
  Node::Input::Axis x;
  Node::Input::Axis y;
  Node::Input::Button left;
  Node::Input::Button right;

  Mouse(Node::Port);

  auto data() -> n2;
  auto latch(n1 data) -> void;

private:
  n1  latched;
  n32 counter;

  n2  speed;  //0 = slow, 1 = normal, 2 = fast
  i32 cx;     //x-coordinate
  i32 cy;     //y-coordinate
  n1  dx;     //x-direction
  n1  dy;     //y-direction
};
