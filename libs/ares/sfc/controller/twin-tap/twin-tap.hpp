struct TwinTap : Controller {
  Node::Input::Button one;
  Node::Input::Button two;

  TwinTap(Node::Port);

  auto data() -> n2;
  auto latch(n1 data) -> void;

private:
  n1 latched;
  n8 counter;
};
