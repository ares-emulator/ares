struct Driving : Controller {
  Node::Input::Axis wheel;
  Node::Input::Button fire;

  Driving(Node::Port);

  auto poll() -> void override;
  auto read() -> n8 override;
  auto serialize(serializer&) -> void override;

private:
  n10 position = 0;
  s8 direction = 0;
};
