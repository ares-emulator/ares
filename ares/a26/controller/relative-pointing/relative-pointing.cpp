RelativePointingDevice::RelativePointingDevice(Node::Port parent, string name, u32 scaleNumerator)
: scaleNumerator(scaleNumerator) {
  node = parent->append<Node::Peripheral>(name);

  x = node->append<Node::Input::Axis>("X");
  y = node->append<Node::Input::Axis>("Y");
  fire = node->append<Node::Input::Button>("Fire");
}

auto RelativePointingDevice::poll() -> void {
  finish(horizontal);
  finish(vertical);

  platform->input(x);
  platform->input(y);
  queue(horizontal, -x->value());
  queue(vertical, -y->value());
}

auto RelativePointingDevice::read() -> n8 {
  advance(horizontal, cpu.clock());
  advance(vertical, cpu.clock());
  platform->input(fire);

  n8 data = 0xff;
  data.bit(0, 3) = encode(horizontal.phase, vertical.phase,
    horizontal.direction < 0, vertical.direction > 0);
  data.bit(4) = !fire->value();
  return data;
}

auto RelativePointingDevice::finish(AxisState& axis) -> void {
  if(axis.remaining) {
    auto offset = axis.direction * (s32)(axis.remaining & 3);
    axis.phase = (axis.phase + 4 + offset) & 3;
  }
  axis.remaining = 0;
  axis.next = 0;
  axis.interval = 1;
}

auto RelativePointingDevice::queue(AxisState& axis, s64 delta) -> void {
  if(delta < -32768) delta = -32768;
  if(delta >  32767) delta =  32767;

  auto scaled = delta * scaleNumerator + axis.remainder;
  auto transitions = scaled < 0
    ? (scaled - ScaleDenominator / 2) / ScaleDenominator
    : (scaled + ScaleDenominator / 2) / ScaleDenominator;
  axis.remainder = scaled - transitions * ScaleDenominator;
  if(transitions < -(s64)MaximumTransitions) transitions = -(s64)MaximumTransitions;
  if(transitions >  (s64)MaximumTransitions) transitions =  (s64)MaximumTransitions;
  if(!transitions) return;

  axis.direction = transitions < 0 ? -1 : 1;
  axis.remaining = abs(transitions);
  axis.interval = max((u64)1, windowDuration() / axis.remaining);
  axis.next = axis.interval;
}

auto RelativePointingDevice::advance(AxisState& axis, u64 timestamp) -> void {
  if(!axis.remaining || timestamp < axis.next) return;

  auto due = 1 + (timestamp - axis.next) / axis.interval;
  if(due > axis.remaining) due = axis.remaining;
  auto offset = axis.direction * (s32)(due & 3);
  axis.phase = (axis.phase + 4 + offset) & 3;
  axis.remaining -= due;
  axis.next += axis.interval * due;
}

auto RelativePointingDevice::windowDuration() const -> u64 {
  return cpu.scalar() * 228 * (Region::NTSC() ? 262 : 312);
}

auto RelativePointingDevice::serialize(serializer& s) -> void {
  for(auto* axis : {&horizontal, &vertical}) {
    s(axis->phase);
    s(axis->direction);
    s(axis->remaining);
    s(axis->next);
    s(axis->interval);
    s(axis->remainder);
  }
}
