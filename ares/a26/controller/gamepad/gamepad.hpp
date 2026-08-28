struct Gamepad : Controller {
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button fire;

  Gamepad(Node::Port);

  auto read() -> n8 override;
};
