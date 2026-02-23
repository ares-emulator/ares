SportsPad::SportsPad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Sports Pad");

  x   = node->append<Node::Input::Axis>  ("X");
  y   = node->append<Node::Input::Axis>  ("Y");
  one = node->append<Node::Input::Button>("1");
  two = node->append<Node::Input::Button>("2");

  Thread::create(1'000'000, std::bind_front(&SportsPad::main, this));
}

SportsPad::~SportsPad() {
  Thread::destroy();
}

auto SportsPad::main() -> void {
  // Timeout is reset when TH falls low.
  // Sampling occurs when TH was kept low
  // for more than 60us.
  if (!th && (timeout > 0)) {
      timeout--;

      if (timeout == 0) {
          index = 0;

          platform->input(x);
          platform->input(y);

          i16 ax = -x->value();
          i16 ay = -y->value();

          if (ax > maxspeed) {
            ax = maxspeed;
          } else if (ax < -maxspeed) {
            ax = -maxspeed;
          }

          if (ay > maxspeed) {
            ay = maxspeed;
          } else if (ay < -maxspeed) {
            ay = -maxspeed;
          }

          status[0] = ax.bit(4,7);
          status[1] = ax.bit(0,3);
          status[2] = ay.bit(4,7);
          status[3] = ay.bit(0,3);
      }
  }
  Thread::step(1);
  Thread::synchronize(cpu);
}

auto SportsPad::read() -> n7 {
  platform->input(one);
  platform->input(two);

  n7 data;

  data.bit(0,3) = status[index];
  data.bit(4) = !one->value();
  data.bit(5) = !two->value();
  data.bit(6) = 1;

  return data;
}

auto SportsPad::write(n4 data) -> void {

  // Standard read sequence is:
  //
  //  1. TH low,  wait 80us, read nibble 1
  //  2. TH high, wait 40us, read nibble 2
  //  3. TH low,  wait 40us, read nibble 3
  //  2. TH high, wait 40us, read nibble 4
  //
  // The longer wait at step 1 is used for
  // synchronisation. In this implementation,
  // the nibble index resets after 60us.

  // Falling edge of TH
  if (th && !data.bit(3)) {
    timeout = 60;
    index++;
  }

  // Rising edge of TH
  if (!th && data.bit(3)) {
    index++;
  }

  if (index > 3) {
      index = 0;
  }

  th = data.bit(3);
}

