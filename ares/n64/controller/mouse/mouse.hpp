struct Mouse : Controller {
  Node::Input::Axis x;
  Node::Input::Axis y;
  Node::Input::Button left;
  Node::Input::Button right;

  Mouse(Node::Port);
  ~Mouse();

  auto read() -> n32 override;
  auto readId() -> u16 override;
};
