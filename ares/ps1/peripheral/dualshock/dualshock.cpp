// NOTE: A real Dual Shock starts up in Digital Mode
// However, we start up in Analog Mode due to there not being
// enough buttons to properly map the "Analog" button on the controller

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

  analogMode = 1;
  newRumbleMode = 0;
  configMode = 0;
}

auto DualShock::reset() -> void {
  state = State::Idle;
  _active = false;
  outputData.clear();
}

auto DualShock::acknowledge() -> bool {
  return state != State::Idle;
}

auto DualShock::active() -> bool {
  return _active || acknowledge();
}

auto DualShock::bus(u8 data) -> u8 {
  n8 input  = data;
  n8 output = 0xff;

  //old rumble mode
  if(!newRumbleMode && command == 0x42) {
    switch(commandStep) {
      case 1: inputData.clear(); inputData.push_back(input); break;
      case 2: rumble->setEnable(inputData[0].bit(6, 7) == 1 && input.bit(0) == 1); break;
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
  if(command == 0x43 && commandStep == 1) {
    configMode = input;
    newRumbleMode = 1;
  }

  //set led state
  if(command == 0x44 && commandStep == 1) {
    //TODO: set LED state to commandStep[1] if commandStep[2] is set
    for(auto n : range(6)) rumbleConfig[n] = 0xff;
  }

  //variable response A
  if(command == 0x46 && commandStep == 1) {
    if(input == 0x00)      { outputData.insert(outputData.end(), {0x01,0x02,0x00,0x00}); }
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

    output = 0xff;
    state = State::IDLower;
    _active = true;
    break;
  }

  case State::IDLower: {
    command = input;

    if(configMode) output = 0xf3;
    else output = analogMode ? 0x73 : 0x41;
    outputData.push_back(0x5a);

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
            case 0x4c: { outputData.insert(outputData.end(), {0x00,0x00,0x00}); } break; // Partial response, will be completed on step 1
            case 0x4d: for(auto n : range(6)) outputData.push_back(rumbleConfig[n]); break;
            default:
              outputData.clear();
              output = invalid(input);
              break;
          }
          break;
        }

        outputData.clear();
        output = invalid(input);
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

auto DualShock::invalid(u8 data) -> u8 {
  debug(unusual, "[DualShock] Invalid command byte ", hex(data));
  _active = false;
  state = State::Idle;
  return 0xff;
}
