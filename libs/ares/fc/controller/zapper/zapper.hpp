struct Zapper : Controller {
  Node::Input::Axis x;
  Node::Input::Axis y;
  Node::Input::Button trigger;
  Node::Video::Sprite sprite;

  Zapper(Node::Port);
  ~Zapper();

  auto main() -> void;
  auto data() -> n3 override;
  auto latch(n1 data) -> void override;
  auto serialize(serializer&) -> void override;

private:
  i32 cx = 256 / 2;     //x-coordinate
  i32 cy = 240 / 2;     //y-coordinate
  i32 px = 0;
  i32 py = 0;
  u32 nx = 256 / 2;
  u32 ny = 240 / 2;
  u32 previous = 0;
};
