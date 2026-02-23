struct LightPhaser : Controller {
  Node::Video::Sprite sprite;
  Node::Input::Axis x;
  Node::Input::Axis y;
  Node::Input::Button trigger;

  LightPhaser(Node::Port);
  ~LightPhaser();

  auto read() -> n7 override;
};
