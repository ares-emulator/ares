MindLink::MindLink(Node::Port parent) {
  node = parent->append<Node::Peripheral>("MindLink");

  x = node->append<Node::Input::Axis>("X");
  trigger = node->append<Node::Input::Button>("Trigger");
}

auto MindLink::poll() -> void {
  platform->input(x);
  platform->input(trigger);

  auto next = (s64)(position & ~TriggerValue) + x->value() * PositionScale;
  position = std::clamp<s64>(next, MinimumPosition, MaximumPosition);

  //Stella exposes this compatibility trigger, but the physical MindLink had no button.
  if(trigger->value()) position |= TriggerValue;

  lines = 0x1f;
  shift = 1;
  nextBit();
}

auto MindLink::read() -> n8 {
  n8 data = 0xff;
  data.bit(0, 4) = lines;
  return data;
}

auto MindLink::write(n8 data) -> void {
  lines.bit(0, 3) = data.bit(0, 3);
}

auto MindLink::controlWrite(n8 data) -> void {
  if(data.bit(0)) nextBit();
}

auto MindLink::serialize(serializer& s) -> void {
  s(position);
  s(shift);
  s(lines);
}

auto MindLink::nextBit() -> void {
  lines.bit(2) = 0;
  lines.bit(3) = (position & shift) != 0;
  shift <<= 1;
}
