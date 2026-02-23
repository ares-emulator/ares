Paddle::Paddle(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Paddle");

  button = node->append<Node::Input::Button>("Button");
  axis   = node->append<Node::Input::Axis>  ("X-Axis");

  // 18kHz - As measured on real hardware
  Thread::create(18000, std::bind_front(&Paddle::main, this));
}

Paddle::~Paddle() {
  Thread::destroy();
}

auto Paddle::main() -> void {
  secondNibble = !secondNibble;

  Thread::step(1);
  Thread::synchronize(cpu);
}

auto Paddle::read() -> n7 {
  platform->input(button);
  platform->input(axis);

  n7 data;
  if (secondNibble) {
    data.bit(0,3) = value.bit(4,7);
  } else {
    // scale {-32768 ... +32767} to {0 ... 255 }
    value = (axis->value() + 32768.0) * 255.0 / 65535.0;
    data.bit(0,3) = value.bit(0,3);
  }

  data.bit(4) = !button->value();
  data.bit(5) = secondNibble;
  data.bit(6) = 1;

  return data;
}
