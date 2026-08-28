Driving::Driving(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Driving");

  wheel = node->append<Node::Input::Axis>("Wheel");
  fire = node->append<Node::Input::Button>("Fire");
}

auto Driving::poll() -> void {
  platform->input(wheel);

  auto value = wheel->value();
  s32 nextDirection = value < -16384 ? -1 : value > 16384 ? 1 : 0;
  if(nextDirection && nextDirection != direction) {
    auto phase = position.bit(8, 9);
    if(nextDirection < 0) position = (phase + 4) * 256 - 1;
    if(nextDirection > 0) position = (phase + 1) * 256;
  } else if(nextDirection) {
    auto distance = value / 512 + (value > 0);
    position = (position + distance + 1024) & 1023;
  }
  direction = nextDirection;
}

auto Driving::read() -> n8 {
  platform->input(fire);

  static constexpr u8 gray[4] = {0x03, 0x01, 0x00, 0x02};

  n8 data = 0xff;
  data.bit(0, 1) = gray[position.bit(8, 9)];
  data.bit(4) = !fire->value();
  return data;
}

auto Driving::serialize(serializer& s) -> void {
  s(position);
  s(direction);
}
