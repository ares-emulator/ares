DualShock::DualShock(Node::Port parent) {
  node = parent->append<Node::Peripheral>("DualShock");

  axis = node->append<Node::Input::Axis>("Axis");

  lx       = node->append<Node::Input::Axis  >("L-Stick X");
  ly       = node->append<Node::Input::Axis  >("L-Stick Y");
  rx       = node->append<Node::Input::Axis  >("R-Stick X");
  ry       = node->append<Node::Input::Axis  >("R-Stick Y");
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
  l3       = node->append<Node::Input::Button>("L3");
  r1       = node->append<Node::Input::Button>("R1");
  r2       = node->append<Node::Input::Button>("R2");
  r3       = node->append<Node::Input::Button>("R3");
  select   = node->append<Node::Input::Button>("Select");
  start    = node->append<Node::Input::Button>("Start");
  mode     = node->append<Node::Input::Button>("Mode");
  rumble   = node->append<Node::Input::Rumble>("Rumble");

  power(false);
}

auto DualShock::power(bool reset) -> void {
  if(reset) return;
  this->reset();
  analogMode = 0;
  newRumbleMode = 0;
  configMode = 0;
  modeLocked = modePressed = false;
  watchdogEnabled = false;
  watchdogRemaining = 0;
  statusByte = 0x5a;
  command = commandStep = 0;
  for(auto& value : rumbleConfig) value = 0xff;
  inputData.clear();
  rumble->setValues(0, 0);
}

auto DualShock::toggleMode() -> void {
  if(modeLocked) return;
  if(newRumbleMode) statusByte = 0;
  analogMode ^= 1;
  configMode = 0;
  for(auto& value : rumbleConfig) value = 0xff;
  rumble->setValues(0, 0);
  platform->input(rumble);
}

auto DualShock::pollMode() -> void {
  platform->input(mode);
  bool pressed = mode->value();
  if(pressed && !modePressed) toggleMode();
  modePressed = pressed;
}

auto DualShock::selectChanged(bool selected) -> void {
  reset();
  if(selected && watchdogEnabled) watchdogRemaining = system.frequency() * 5 / 2;
}

auto DualShock::step(u32 clocks) -> void {
  if(!watchdogRemaining) return;
  watchdogRemaining -= min(watchdogRemaining, clocks);
  if(watchdogRemaining) return;
  bool pressed = modePressed;
  power(false);
  modePressed = pressed;
  newRumbleMode = 1;
  platform->input(rumble);
}

auto DualShock::reset() -> void {
  state = State::Idle;
  _active = false;
  outputData.clear();
  inputData.clear();
  command = commandStep = 0;
}

auto DualShock::serialize(serializer& s) -> void {
  s(state);
  s(analogMode);
  s(newRumbleMode);
  s(configMode);
  s(modeLocked);
  s(modePressed);
  s(watchdogEnabled);
  s(watchdogRemaining);
  s(statusByte);
  s(command);
  s(commandStep);
  s(rumbleConfig);
  s(_active);
  auto queue = [&](auto& values) {
    u8 count = values.size();
    s(count);
    if(s.reading()) values.clear();
    for(u32 index : range(8)) {
      u8 value = s.writing() && index < values.size() ? (u8)values[index] : 0;
      s(value);
      if(s.reading() && index < count) values.push_back(value);
    }
  };
  queue(outputData);
  queue(inputData);
}

auto DualShock::acknowledge() -> u32 {
  return state != State::Idle ? 338 : 0;
}

auto DualShock::active() -> bool {
  return _active || state != State::Idle;
}

auto DualShock::bus(u8 data) -> u8 {
  n8 input  = data;
  n8 output = 0xff;

  //old rumble mode
  if(!newRumbleMode && command == 0x42) {
    switch(commandStep) {
      case 1: inputData.clear(); inputData.push_back(input); break;
      case 2: rumble->setValues(0, inputData[0].bit(6, 7) == 1 && input.bit(0) == 1 ? 0xffff : 0); break;
    }
    platform->input(rumble);
  }

  //new rumble mode
  if(newRumbleMode && command == 0x42 && commandStep > 0) {
    auto index = commandStep - 1;
    if(rumbleConfig[index] == 0x00) rumble->setWeak(input.bit(0) ? 0xffff : 0); // small motor
    if(rumbleConfig[index] == 0x01) rumble->setStrong(input * 65535 / 255);     // large motor
    platform->input(rumble);
  }

  //config Mode Enable/Disable
  if(command == 0x43 && commandStep == 1 && input <= 1) {
    configMode = input != 0;
    newRumbleMode = 1;
    if(input == 1) {
      statusByte = 0x5a;
      watchdogEnabled = true;
      watchdogRemaining = system.frequency() * 5 / 2;
    }
  }

  //set led state
  if(command == 0x44 && commandStep == 1) {
    if(input <= 1) {
      analogMode = input;
    } else if(outputData.size() > 2) {
      outputData[2] = 0xff;
    }
  }
  if(command == 0x44 && commandStep == 2) modeLocked = (input & 3) == 3;

  //variable response A
  if(command == 0x46 && commandStep == 1) {
    if(input == 0x00)      { outputData.insert(outputData.end(), {0x01,0x02,0x00,0x0a}); }
    else if(input == 0x01) { outputData.insert(outputData.end(), {0x01,0x01,0x01,0x14}); }
    else                   { outputData.insert(outputData.end(), {0x00,0x00,0x00,0x00}); }
  }

  //variable response B
  if(command == 0x4c && commandStep == 1) {
    u8 value = 0x00;
    if(input == 0x00) value = 0x04;
    if(input == 0x01) value = 0x07;
    outputData.insert(outputData.end(), {value,0x00,0x00});
  }

  if(command == 0x4d && commandStep >= 1) {
    rumbleConfig[commandStep - 1] = input;
    if(commandStep == 6 && inputData.size() == 6) {
      bool query = true;
      for(auto value : rumbleConfig) query &= value == 0xff;
      if(query) for(u32 index : range(6)) rumbleConfig[index] = inputData[index];
    }
  }

  //if there is data in the output queue, return that
  if(outputData.size() > 0) {
    output = outputData.front();
    outputData.erase(outputData.begin());
    commandStep++;
    if(outputData.size() == 0) {
      commandStep = 0;
      state = State::Idle;
    }
    return output;
  }

  switch(state) {

  case State::Idle: {
    command = 0;
    if(input != 0x01) {
      _active = false;
      break;
    }

    pollMode();
    output = 0xff;
    state = State::IDLower;
    _active = true;
    break;
  }

  case State::IDLower: {
    command = input;

    if(configMode) output = 0xf3;
    else output = analogMode ? 0x73 : 0x41;
    outputData.push_back(statusByte);

    //Global commands: these work during any operation mode
    switch(input) {
      case 0x42: { 
        auto v = readPad();
        outputData.insert(outputData.end(), v.begin(), v.end()); 
      } break;
      case 0x43: {
        if(configMode) { outputData.insert(outputData.end(), {0x00,0x00,0x00,0x00,0x00,0x00}); }
        else {
          auto v = readPad();
          outputData.insert(outputData.end(), v.begin(), v.end());
        } break;
      default:
        if(configMode) {
          switch(input) {
            case 0x44: { outputData.insert(outputData.end(), {0x00,0x00,0x00,0x00,0x00,0x00}); } break;
            case 0x45: { outputData.insert(outputData.end(), {0x01,0x02,(u8)analogMode,0x02,0x01,0x00}); } break;
            case 0x46: { outputData.insert(outputData.end(), {0x00,0x00}); } break; // Partial response, will be completed on step 1
            case 0x47: { outputData.insert(outputData.end(), {0x00,0x00,0x02,0x00,0x01,0x00}); } break;
            case 0x48: { outputData.insert(outputData.end(), {0x00,0x00,0x00,0x00,0x01,0x00}); } break;
            case 0x4c: { outputData.insert(outputData.end(), {0x00,0x00,0x00}); } break; // Partial response, will be completed on step 1
            case 0x4d:
              inputData.clear();
              for(auto n : range(6)) {
                outputData.push_back(rumbleConfig[n]);
                inputData.push_back(rumbleConfig[n]);
              }
              break;
            default:
              if(input >= 0x40 && input <= 0x4f) {
                outputData.insert(outputData.end(), {0x00,0x00,0x00,0x00,0x00,0x00});
              } else {
                outputData.clear();
                invalid(input);
              }
              break;
          }
          break;
        }

        outputData.clear();
        invalid(input);
        break;
      }
    }
    break;
  }

  }

  return output;
}

auto DualShock::readPad() -> std::vector<u8> {
  std::vector<u8> result;
  n8 output;

  platform->input(select);
  platform->input(l3);
  platform->input(r3);
  platform->input(start);
  platform->input(up);
  platform->input(right);
  platform->input(down);
  platform->input(left);

  output.bit(0) = !select->value();
  output.bit(1) = analogMode || configMode ? !l3->value() : 1;
  output.bit(2) = analogMode || configMode ? !r3->value() : 1;
  output.bit(3) = !start->value();
  output.bit(4) = !(up->value() & !down->value());
  output.bit(5) = !(right->value() & !left->value());
  output.bit(6) = !(down->value() & !up->value());
  output.bit(7) = !(left->value() & !right->value());
  result.push_back(output);

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
  result.push_back(output);

  if(!analogMode && !configMode) return result;

  auto cardinalMax = 127.5;
  auto diagonalMax = 127.5;
  auto innerDeadzone = 6.0; //missing information on deadzone for DualShock potentiometers; use arbitrary number roughly 4.7% of cardinalMax instead (common software default in gaming industry seems to be 5.0%) 
  auto saturationRadius = (innerDeadzone + diagonalMax + sqrt(pow(innerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * innerDeadzone)) / sqrt(2.0); //from linear response curve function within axis->processDeadzoneAndResponseCurve, substitute saturationRadius * sqrt(2) / 2 for right-hand lengthAbsolute and set diagonalMax as the result then solve for saturationRadius
  auto offset = -0.5;

  platform->input(rx);
  platform->input(ry);

  //scale {-32767 ... +32767} to {-saturationRadius + offset ... +saturationRadius + offset}
  auto arx = axis->setOperatingRange(rx->value(), saturationRadius, offset);
  auto ary = axis->setOperatingRange(ry->value(), saturationRadius, offset);

  //create inner axial dead-zone in range {-innerDeadzone ... +innerDeadzone} and scale from it up to saturationRadius
  arx = axis->processDeadzoneAndResponseCurve(arx, innerDeadzone, saturationRadius, offset);
  ary = axis->processDeadzoneAndResponseCurve(ary, innerDeadzone, saturationRadius, offset);

  auto scaledLengthRightStick = hypot(arx - offset, ary - offset);
  if(scaledLengthRightStick > saturationRadius) {
    arx = axis->revisePosition(arx, scaledLengthRightStick, saturationRadius, offset);
    ary = axis->revisePosition(ary, scaledLengthRightStick, saturationRadius, offset);
  }

  //let cardinalMax and diagonalMax define boundaries and restrict to a square gate
  double arxBounded = 0.0;
  double aryBounded = 0.0;
  axis->applyGateBoundaries(innerDeadzone, cardinalMax, diagonalMax, arx, ary, offset, arxBounded, aryBounded);
  arx = arxBounded;
  ary = aryBounded;

  //keep cardinal input within positive and negative bounds of cardinalMax
  arx = axis->clampAxisToNearestBoundary(arx, offset, cardinalMax);
  ary = axis->clampAxisToNearestBoundary(ary, offset, cardinalMax);

  //add epsilon to counteract floating point precision error
  arx = axis->counteractPrecisionError(arx);
  ary = axis->counteractPrecisionError(ary);

  result.push_back(u8(arx + 128.0));
  result.push_back(u8(ary + 128.0));

  platform->input(lx);
  platform->input(ly);

  auto alx = axis->setOperatingRange(lx->value(), saturationRadius, offset);
  auto aly = axis->setOperatingRange(ly->value(), saturationRadius, offset);

  alx = axis->processDeadzoneAndResponseCurve(alx, innerDeadzone, saturationRadius, offset);
  aly = axis->processDeadzoneAndResponseCurve(aly, innerDeadzone, saturationRadius, offset);

  auto scaledLengthLeftStick = hypot(alx - offset, aly - offset);
  if(scaledLengthLeftStick > saturationRadius) {
    alx = axis->revisePosition(alx, scaledLengthLeftStick, saturationRadius, offset);
    aly = axis->revisePosition(aly, scaledLengthLeftStick, saturationRadius, offset);
  }

  double alxBounded = 0.0;
  double alyBounded = 0.0;
  axis->applyGateBoundaries(innerDeadzone, cardinalMax, diagonalMax, alx , aly, offset, alxBounded, alyBounded);
  alx = alxBounded;
  aly = alyBounded;

  alx = axis->clampAxisToNearestBoundary(alx, offset, cardinalMax);
  aly = axis->clampAxisToNearestBoundary(aly, offset, cardinalMax);

  alx = axis->counteractPrecisionError(alx);
  aly = axis->counteractPrecisionError(aly);

  result.push_back(u8(alx + 128.0));
  result.push_back(u8(aly + 128.0));

  return result;
}

auto DualShock::invalid(u8 data) -> void {
  debug(unusual, "[DualShock] Invalid command byte ", hex(data));
  _active = false;
  state = State::Idle;
}
