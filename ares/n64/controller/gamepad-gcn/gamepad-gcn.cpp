GamepadGCN::GamepadGCN(Node::Port parent) {
  node = parent->append<Node::Peripheral>("GameCube Gamepad");

  stickAxis  = node->append<Node::Input::Axis>("Axis");
  cstickAxis = node->append<Node::Input::Axis>("C-Axis");

  x       = node->append<Node::Input::Axis>  ("X-Axis");
  y       = node->append<Node::Input::Axis>  ("Y-Axis");
  cx      = node->append<Node::Input::Axis>  ("C-X-Axis");
  cy      = node->append<Node::Input::Axis>  ("C-Y-Axis");
  lAnalog = node->append<Node::Input::Axis>  ("L-Analog");
  rAnalog = node->append<Node::Input::Axis>  ("R-Analog");
  up      = node->append<Node::Input::Button>("Up");
  down    = node->append<Node::Input::Button>("Down");
  left    = node->append<Node::Input::Button>("Left");
  right   = node->append<Node::Input::Button>("Right");
  a       = node->append<Node::Input::Button>("A");
  b       = node->append<Node::Input::Button>("B");
  xButton = node->append<Node::Input::Button>("X");
  yButton = node->append<Node::Input::Button>("Y");
  l       = node->append<Node::Input::Button>("L");
  r       = node->append<Node::Input::Button>("R");
  z       = node->append<Node::Input::Button>("Z");
  start   = node->append<Node::Input::Button>("Start");
  motor   = node->append<Node::Input::Rumble>("Rumble");

  reset();
}

GamepadGCN::~GamepadGCN() {
  rumble(false);
}

auto GamepadGCN::reset() -> void {
  rumble(false);
  originPending = 1;
  state = {};
  state.stickX  = OriginStick;
  state.stickY  = OriginStick;
  state.cstickX = OriginCStick;
  state.cstickY = OriginCStick;
  state.analogL = OriginTrigger;
  state.analogR = OriginTrigger;
}

auto GamepadGCN::rumble(bool enable) -> void {
  if(!motor) return;
  motor->setEnable(enable);
  platform->input(motor);
}

//sample the host controller and convert it to raw GameCube readings.
//
//the console subtracts the origin from every axis, so the values written here
//are origin-relative: neutral must read exactly as the origin, and full
//deflection must land on the range libdragon normalizes against.
auto GamepadGCN::poll() -> void {
  platform->input(x);
  platform->input(y);
  platform->input(cx);
  platform->input(cy);
  platform->input(lAnalog);
  platform->input(rAnalog);
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(a);
  platform->input(b);
  platform->input(xButton);
  platform->input(yButton);
  platform->input(l);
  platform->input(r);
  platform->input(z);
  platform->input(start);

  //the GameCube stick gate is octagonal like the N64's; cardinal and diagonal
  //limits are taken from the physical gate, scaled to libdragon's stick range
  auto cardinalMax = RangeStick;
  auto diagonalMax = RangeStick * 0.75;
  auto innerDeadzone = RangeStick * 0.08;
  auto saturationRadius = (innerDeadzone + diagonalMax + sqrt(pow(innerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * innerDeadzone)) / sqrt(2.0);
  auto offset = 0.0;

  auto ax = stickAxis->setOperatingRange(x->value(), saturationRadius, offset);
  auto ay = stickAxis->setOperatingRange(y->value(), saturationRadius, offset);

  ax = stickAxis->processDeadzoneAndResponseCurve(ax, innerDeadzone, saturationRadius, offset);
  ay = stickAxis->processDeadzoneAndResponseCurve(ay, innerDeadzone, saturationRadius, offset);

  auto scaledLength = hypot(ax - offset, ay - offset);
  if(scaledLength > saturationRadius) {
    ax = stickAxis->revisePosition(ax, scaledLength, saturationRadius, offset);
    ay = stickAxis->revisePosition(ay, scaledLength, saturationRadius, offset);
  }

  double axBounded = 0.0;
  double ayBounded = 0.0;
  stickAxis->applyGateBoundaries(innerDeadzone, cardinalMax, diagonalMax, ax, ay, offset, axBounded, ayBounded);
  ax = stickAxis->clampAxisToNearestBoundary(axBounded, offset, cardinalMax);
  ay = stickAxis->clampAxisToNearestBoundary(ayBounded, offset, cardinalMax);

  ax = stickAxis->counteractPrecisionError(ax);
  ay = stickAxis->counteractPrecisionError(ay);

  //the C-stick has its own smaller range and gate
  auto cCardinalMax = RangeCStick;
  auto cDiagonalMax = RangeCStick * 0.75;
  auto cInnerDeadzone = RangeCStick * 0.08;
  auto cSaturationRadius = (cInnerDeadzone + cDiagonalMax + sqrt(pow(cInnerDeadzone + cDiagonalMax, 2.0) - 2.0 * sqrt(2.0) * cDiagonalMax * cInnerDeadzone)) / sqrt(2.0);

  auto bx = cstickAxis->setOperatingRange(cx->value(), cSaturationRadius, offset);
  auto by = cstickAxis->setOperatingRange(cy->value(), cSaturationRadius, offset);

  bx = cstickAxis->processDeadzoneAndResponseCurve(bx, cInnerDeadzone, cSaturationRadius, offset);
  by = cstickAxis->processDeadzoneAndResponseCurve(by, cInnerDeadzone, cSaturationRadius, offset);

  auto cScaledLength = hypot(bx - offset, by - offset);
  if(cScaledLength > cSaturationRadius) {
    bx = cstickAxis->revisePosition(bx, cScaledLength, cSaturationRadius, offset);
    by = cstickAxis->revisePosition(by, cScaledLength, cSaturationRadius, offset);
  }

  double bxBounded = 0.0;
  double byBounded = 0.0;
  cstickAxis->applyGateBoundaries(cInnerDeadzone, cCardinalMax, cDiagonalMax, bx, by, offset, bxBounded, byBounded);
  bx = cstickAxis->clampAxisToNearestBoundary(bxBounded, offset, cCardinalMax);
  by = cstickAxis->clampAxisToNearestBoundary(byBounded, offset, cCardinalMax);

  bx = cstickAxis->counteractPrecisionError(bx);
  by = cstickAxis->counteractPrecisionError(by);

  //triggers rest at the origin and travel in one direction only; the host
  //reports them released at zero and fully depressed at the positive maximum
  auto triggerValue = [&](Node::Input::Axis axis) -> u8 {
    f64 position = axis->value() / 32767.0 * RangeTrigger;
    if(position < 0.0) position = 0.0;
    if(position > RangeTrigger) position = RangeTrigger;
    return u8(OriginTrigger + position);
  };

  //the host reports up as negative, while the controller reports it as
  //positive above the origin, so the vertical axes are inverted
  state.stickX  = u8(OriginStick  + s8(+ax));
  state.stickY  = u8(OriginStick  + s8(-ay));
  state.cstickX = u8(OriginCStick + s8(+bx));
  state.cstickY = u8(OriginCStick + s8(-by));
  state.analogL = triggerValue(lAnalog);
  state.analogR = triggerValue(rAnalog);

  state.a     = a->value();
  state.b     = b->value();
  state.x     = xButton->value();
  state.y     = yButton->value();
  state.z     = z->value();
  state.start = start->value();

  //a GameCube trigger is analog for most of its travel and clicks a separate
  //switch at the very bottom. below the threshold only the analog reading
  //moves; crossing it bottoms the analog value out and presses the button,
  //which is what the click on real hardware does.
  auto clickThreshold = u8(OriginTrigger + RangeTrigger * TriggerThreshold);

  state.l = l->value() || state.analogL >= clickThreshold;
  state.r = r->value() || state.analogR >= clickThreshold;

  if(state.l) state.analogL = u8(OriginTrigger + RangeTrigger);
  if(state.r) state.analogR = u8(OriginTrigger + RangeTrigger);

  //a keyboard or a remapped pad can report opposing directions at once, which
  //a d-pad in working order does not; suppress the pair for consistency with
  //the N64 Gamepad, which already filters them the same way
  state.up    = up->value()    & !down->value();
  state.down  = down->value()  & !up->value();
  state.left  = left->value()  & !right->value();
  state.right = right->value() & !left->value();
}

//the two button bytes are identical in every analog mode
auto GamepadGCN::writeButtons(n8 output[]) -> void {
  output[0] = 0x00;
  output[0].bit(5) = originPending;  //check_origin
  output[0].bit(4) = state.start;
  output[0].bit(3) = state.y;
  output[0].bit(2) = state.x;
  output[0].bit(1) = state.b;
  output[0].bit(0) = state.a;

  output[1] = 0x00;
  output[1].bit(7) = 1;  //use_origin
  output[1].bit(6) = state.l;
  output[1].bit(5) = state.r;
  output[1].bit(4) = state.z;
  output[1].bit(3) = state.up;
  output[1].bit(2) = state.down;
  output[1].bit(1) = state.right;
  output[1].bit(0) = state.left;
}

//bytes four through seven carry the C-stick, the triggers and the analog face
//buttons at precisions that vary per mode. a standard controller has no
//pressure sensitive face buttons, so the analog A and B fields are always zero.
auto GamepadGCN::writeAnalogMode(n8 mode, n8 output[]) -> void {
  switch(mode) {

  case 1:
    output[4] = state.cstickX & 0xf0 | state.cstickY >> 4;
    output[5] = state.analogL;
    output[6] = state.analogR;
    output[7] = 0x00;  //analog A and B
    break;

  case 2:
    output[4] = state.cstickX & 0xf0 | state.cstickY >> 4;
    output[5] = state.analogL & 0xf0 | state.analogR >> 4;
    output[6] = 0x00;  //analog A
    output[7] = 0x00;  //analog B
    break;

  case 3:
    output[4] = state.cstickX;
    output[5] = state.cstickY;
    output[6] = state.analogL;
    output[7] = state.analogR;
    break;

  case 4:
    output[4] = state.cstickX;
    output[5] = state.cstickY;
    output[6] = 0x00;  //analog A
    output[7] = 0x00;  //analog B
    break;

  //modes 0, 5, 6 and 7 share the same packing
  default:
    output[4] = state.cstickX;
    output[5] = state.cstickY;
    output[6] = state.analogL & 0xf0 | state.analogR >> 4;
    output[7] = 0x00;  //analog A and B
    break;

  }
}

//the origin block reports the neutral reading of every analog input.
//layout matches the ten byte response of the origin and recalibrate commands.
auto GamepadGCN::writeOrigin(n8 output[]) -> void {
  output[0] = 0x00;
  output[1] = 0x80;  //use_origin
  output[2] = OriginStick;
  output[3] = OriginStick;
  output[4] = OriginCStick;
  output[5] = OriginCStick;
  output[6] = OriginTrigger;
  output[7] = OriginTrigger;
  output[8] = 0x00;  //analog A
  output[9] = 0x00;  //analog B
}

auto GamepadGCN::comm(n8 send, n8 recv, n8 input[], n8 output[]) -> n2 {
  b1 valid = 0;
  b1 over = 0;

  //status
  if(input[0] == 0x00 || input[0] == 0xff) {
    //identifier 0x0900: GameCube platform, acts as a standard controller.
    //the flag that marks a controller as lacking rumble is left clear, and
    //the wireless flag with it.
    output[0] = 0x09;
    output[1] = 0x00;
    //the status byte reports whether the rumble motor is currently running
    output[2] = 0x00;
    output[2].bit(3) = motor && motor->enable();
    valid = 1;
  }

  //read controller state
  if(input[0] == 0x40 && send >= 3) {
    poll();

    //the third byte of the request drives the rumble motor
    rumble(input[2] & 1);

    writeButtons(output);

    //the first two analog bytes hold the stick in every mode; the remaining
    //four are repacked at different precisions depending on the mode, trading
    //resolution between the C-stick, the triggers and the analog face buttons
    output[2] = state.stickX;
    output[3] = state.stickY;
    writeAnalogMode(input[1], output);

    if(recv <= 8) {
      over = 0;
    } else {
      over = 1;
    }
    valid = 1;
  }

  //read origins
  if(input[0] == 0x41) {
    poll();
    writeOrigin(output);
    originPending = 0;
    if(recv <= 10) {
      over = 0;
    } else {
      over = 1;
    }
    valid = 1;
  }

  //recalibrate: the origins are fixed, so this only clears the pending flag
  if(input[0] == 0x42) {
    poll();
    writeOrigin(output);
    originPending = 0;
    if(recv <= 10) {
      over = 0;
    } else {
      over = 1;
    }
    valid = 1;
  }

  //long read: identical to the standard read plus the analog A and B buttons,
  //which a standard controller always reports as zero
  if(input[0] == 0x43) {
    poll();

    writeButtons(output);
    output[2] = state.stickX;
    output[3] = state.stickY;
    output[4] = state.cstickX;
    output[5] = state.cstickY;
    output[6] = state.analogL;
    output[7] = state.analogR;
    output[8] = 0x00;  //analog A
    output[9] = 0x00;  //analog B

    if(recv <= 10) {
      over = 0;
    } else {
      over = 1;
    }
    valid = 1;
  }

  n2 status = 0;
  status.bit(0) = valid;
  status.bit(1) = over;
  return status;
}

auto GamepadGCN::serialize(serializer& s) -> void {
  s(state.stickX);
  s(state.stickY);
  s(state.cstickX);
  s(state.cstickY);
  s(state.analogL);
  s(state.analogR);
  s(state.a);
  s(state.b);
  s(state.x);
  s(state.y);
  s(state.z);
  s(state.l);
  s(state.r);
  s(state.start);
  s(state.up);
  s(state.down);
  s(state.left);
  s(state.right);
  s(originPending);
}
