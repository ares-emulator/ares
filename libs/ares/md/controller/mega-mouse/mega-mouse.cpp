MegaMouse::MegaMouse(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Mega Mouse");

  x      = node->append<Node::Input::Axis>("X");
  y      = node->append<Node::Input::Axis>("Y");
  left   = node->append<Node::Input::Button>("Left");
  right  = node->append<Node::Input::Button>("Right");
  middle = node->append<Node::Input::Button>("Middle");
  start  = node->append<Node::Input::Button>("Start");

  status[0] = 0x0; // TH high
  status[1] = 0xB; // TH low
  status[2] = 0xF; // 1st nibble
  status[3] = 0xF; // 2nd
  // status[4..9]  // 3rd .. 8th nibbles

  // Is is said that some games do not do the hanshake correctly
  // and rely heavily on timing, so we try to mimick the real
  // timing closely.

  // One game has a programming error where it polls for TL low
  // instead of TL high for one of the nibbles so it falls rights
  // through and works by chance. With a 1MHz clock here, vertical
  // cursor motion was intermittent. So 2MHz is used.
  double timerfreq = 2'000'000;

  // time between falling edge of clock (TR) and
  // handshake on TL.
  //
  // For first nibble, the measured time is min. 12us, max 34us
  // For folling nibbles are available in 12-14 us.
  t_handshake = 14 * timerfreq / 1000000;

  // data changes a bit before the TL handshake occurs,
  // precisely 4.8us before. But for some games, 4.8 does not
  // work correclty and it helps to have the data update sooner. So
  // 10 is used here.
  t_data = 10 * timerfreq / 1000000;

  Thread::create(timerfreq, std::bind_front(&MegaMouse::main, this));
}

MegaMouse::~MegaMouse() {
  Thread::destroy();
}

auto MegaMouse::main() -> void {
  // process clocking after building the data to
  // cause at least some lag. In real life, it takes
  // about 10us to the mouse to react and change TL
  // to reflect TR...
  if (timeout)
  {
    timeout--;

    if (timeout == t_data) {
      index++;
      if (index > 9) {
        index = 1;
      }
    }
    if (timeout == 0) {
      tl = tr;
    }
  }
  Thread::step(1);
  Thread::synchronize(cpu);
}

auto MegaMouse::readData() -> Data {
  n8 data;

  if (th) {
    // When TH is high, the mouse drives a constant on the data pins.
    data.bit(0,3) = status[0];
  }
  else {
    // When TH is low, output is clocked
    data.bit(0,3) = status[index];
  }

  data.bit(4) = tl;

  return {data, 0x1f};
}

auto MegaMouse::writeData(n8 data) -> void {
  //todo: this check wouldn't be necessary if the logic below was correct.
  //this function should only respond to level changes.
  if(tr == data.bit(5) && th == data.bit(6)) return;

  // Falling TH
  if (!data.bit(6) && th) {
    // When TH falls low, make sure the second nibble is driven. The
    // index is otherwise controlled by clocking on TR.
    index = 1;

    platform->input(x);
    platform->input(y);
    platform->input(left);
    platform->input(right);
    platform->input(middle);
    platform->input(start);

    // Button byte
    status[4] = 0;
    status[5].bit(0) = left->value();
    status[5].bit(1) = right->value();
    status[5].bit(2) = middle->value();
    status[5].bit(3) = start->value();

    i16 ax = x->value();
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

    // X byte
    status[4].bit(0) = ax.bit(8);
    status[6] = ax.bit(4,7);
    status[7] = ax.bit(0,3);

    // Y byte
    status[4].bit(1) = ay.bit(8);
    status[8] = ay.bit(4,7);
    status[9] = ay.bit(0,3);
  }

  // Clock in is TR. The mouse reacts and ACKs by updating TL,
  // but not instantly.
  tr = data.bit(5);
  if (tr != tl) {
     timeout = t_handshake;
  }

  th = data.bit(6);
}
