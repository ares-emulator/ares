struct TwinTap : Controller {
  Node::Input::Button one;
  Node::Input::Button two;

  TwinTap(Node::Port);

  auto data() -> uint2;
  auto latch(bool data) -> void;

private:
  bool latched = 0;
  uint counter = 0;
};
