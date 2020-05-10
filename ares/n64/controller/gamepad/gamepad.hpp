struct Gamepad : Controller {
  Node::Axis x;
  Node::Axis y;
  Node::Button up;
  Node::Button down;
  Node::Button left;
  Node::Button right;
  Node::Button b;
  Node::Button a;
  Node::Button cameraUp;
  Node::Button cameraDown;
  Node::Button cameraLeft;
  Node::Button cameraRight;
  Node::Button l;
  Node::Button r;
  Node::Button z;
  Node::Button start;

  Gamepad(Node::Port);
  auto read() -> uint32 override;
};
