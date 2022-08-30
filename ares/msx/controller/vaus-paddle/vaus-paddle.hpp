struct VausPaddle : Controller {
  Node::Input::Button button;
  Node::Input::Axis axis;

  VausPaddle(Node::Port);

  auto read() -> n6 override;
  auto write(n8 data) -> void override;

private:
  b1  serialOut;
  b1  clk;
  b1  reset;
  n9  value;
  i16 min_value = 110;
  i16 max_value = 380;
};
