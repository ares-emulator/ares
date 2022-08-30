VausPaddle::VausPaddle(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Arkanoid Vaus Paddle");

  button = node->append<Node::Input::Button>("Button");
  axis   = node->append<Node::Input::Axis>  ("X-Axis");
}

auto VausPaddle::read() -> n6 {
  platform->input(button);

  n6 data;
  data.bit(0) = serialOut;        // pin 1 (serial data)
  data.bit(1) = !button->value(); // pin 2 (button)

  // Reflect the state of the clock line
  data.bit(4) = clk;

  // Unconnected inputs should read as 1 through pull-up resistors
  data.bit(2) = 1;
  data.bit(3) = 1;
  data.bit(5) = 1;

  return data;
}

auto VausPaddle::write(n8 data) -> void {

  // pin 8 is the reset input. Sample on the rising edge.
  if (!reset && data.bit(2)) {
    platform->input(axis);

    // scale {-32768 ... +32767} to {min_value ... max_value}
    value = (axis->value() + 32768.0) * (max_value - min_value) / 65535.0 + min_value;
    serialOut = value.bit(8); // MSb first
  }

  // pin 6 is the clock input. Shift data on the falling edge.
  if (clk && !data.bit(0)) {
    value <<= 1;
    serialOut = value.bit(8);
  }

  reset = data.bit(2);
  clk   = data.bit(0);
}
