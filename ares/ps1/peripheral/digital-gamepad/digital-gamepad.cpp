DigitalGamepad::DigitalGamepad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Digital Gamepad");

  up       = node->append<Node::Input::Button>("Up");
  down     = node->append<Node::Input::Button>("Down");
  left     = node->append<Node::Input::Button>("Left");
  right    = node->append<Node::Input::Button>("Right");
  cross    = node->append<Node::Input::Button>("Cross");
  circle   = node->append<Node::Input::Button>("Circle");
  square   = node->append<Node::Input::Button>("Square");
  triangle = node->append<Node::Input::Button>("Triangle");
  l1       = node->append<Node::Input::Button>("L1");
  l2       = node->append<Node::Input::Button>("L2");
  r1       = node->append<Node::Input::Button>("R1");
  r2       = node->append<Node::Input::Button>("R2");
  select   = node->append<Node::Input::Button>("Select");
  start    = node->append<Node::Input::Button>("Start");
}

auto DigitalGamepad::reset() -> void {
  state = State::Idle;
}

auto DigitalGamepad::acknowledge() -> bool {
  return state != State::Idle;
}

auto DigitalGamepad::bus(u8 data) -> u8 {
  n8 input  = data;
  n8 output = 0xff;

  switch(state) {

  case State::Idle: {
    if(input != 0x01) break;
    output = 0xff;
    state = State::IDLower;
    break;
  }

  case State::IDLower: {
    if(input != 0x42) break;
    output = 0x41;
    state = State::IDUpper;
    break;
  }

  case State::IDUpper: {
    output = 0x5a;
    state = State::DataLower;
    break;
  }

  case State::DataLower: {
    platform->input(select);
    platform->input(start);
    platform->input(up);
    platform->input(right);
    platform->input(down);
    platform->input(left);

    output.bit(0) = !select->value();
    output.bit(1) = 1;
    output.bit(2) = 1;
    output.bit(3) = !start->value();
    output.bit(4) = !(up->value() & !down->value());
    output.bit(5) = !(right->value() & !left->value());
    output.bit(6) = !(down->value() & !up->value());
    output.bit(7) = !(left->value() & !right->value());
    state = State::DataUpper;
    break;
  }

  case State::DataUpper: {
    platform->input(l2);
    platform->input(r2);
    platform->input(l1);
    platform->input(r1);
    platform->input(triangle);
    platform->input(circle);
    platform->input(cross);
    platform->input(square);

    output.bit(0) = !l2->value();
    output.bit(1) = !r2->value();
    output.bit(2) = !l1->value();
    output.bit(3) = !r1->value();
    output.bit(4) = !triangle->value();
    output.bit(5) = !circle->value();
    output.bit(6) = !cross->value();
    output.bit(7) = !square->value();
    state = State::Release;
    break;
  }

  case State::Release: {
    output = 0xff;
    state = State::Idle;
    break;
  }

  }

  return output;
}
