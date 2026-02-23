struct Paddle : Controller, Thread {
  Node::Input::Axis axis;
  Node::Input::Button button;

  Paddle(Node::Port);
  ~Paddle();

  auto main() -> void;
  auto read() -> n7 override;

private:
  n8 value;
  b1 secondNibble;
};
