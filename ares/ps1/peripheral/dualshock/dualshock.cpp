// NOTE: A real Dual Shock starts up in Digital Mode
// However, we start up in Analog Mode due to there not being
// enough buttons to properly map the "Analog" button on the controller

DualShock::DualShock(Node::Port parent) {
  node = parent->append<Node::Peripheral>("DualShock");

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
}

auto DualShock::reset() -> void {
  state = State::Idle;
  _active = false;
  outputData.reset();
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

  //config Mode Enable/Disable
  if(command == 0x43 && commandStep == 1) {
    configMode = input;
  }

  //set led state
  if(command == 0x44 && commandStep == 1) {
    //TODO: set LED state to commandStep[1] if commandStep[2] is set
    for(auto n : range(6)) rumbleConfig[n] = 0xff;
  }

  //variable response A
  if(command == 0x46 && commandStep == 1) {
    if(input == 0x00)      outputData.append({0x01, 0x02, 0x00, 0x00});
    else if(input == 0x01) outputData.append({0x01, 0x01, 0x01, 0x14});
    else                   outputData.append({0x00, 0x00, 0x00, 0x00});
  }

  //variable response B
  if(command == 0x4c && commandStep == 1) {
    u8 value = 0x00;
    if(input == 0x00) value = 0x04;
    if(input == 0x01) value = 0x07;
    outputData.append({value, 0x00, 0x00});
  }

  if(command == 0x4d && commandStep >= 1) {
    rumbleConfig[commandStep - 1] = input;
  }

  //if there is data in the output queue, return that
  if(outputData.size() > 0) {
    output = outputData.takeFirst();
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
    outputData.append(0x5a);

    switch(input) {
      case 0x42: outputData.append(readPad()); break;
      case 0x43: {
        if(configMode) outputData.append({0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
        else outputData.append(readPad());
        break;
      }
      case 0x44: outputData.append({0x00, 0x00, 0x00, 0x00, 0x00, 0x00}); break;
      case 0x45: outputData.append({0x01, 0x02, analogMode, 0x02, 0x01, 0x00}); break;
      case 0x46: outputData.append({0x00, 0x00}); break; // Partial response, will be completed on step 1
      case 0x47: outputData.append({0x00, 0x00, 0x02, 0x00, 0x01, 0x00}); break;
      case 0x4c: outputData.append({0x00, 0x00, 0x00}); break; // Partial response, will be completed on step 1
      case 0x4d: for(auto n : range(6)) outputData.append(rumbleConfig[n]); break;
      default:
        outputData.reset();
        output = invalid(input);
        break;
    }
    break;
  }

  }

  return output;
}

auto DualShock::readPad() -> vector<u8> {
  vector<u8> result;
  n8 output;

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
  result.append(output);

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
  result.append(output);

  if(!analogMode && !configMode) return result;

  platform->input(rx);
  platform->input(ry);
  result.append((rx->value() + 32768) * 255 / 65535);
  result.append((ry->value() + 32768) * 255 / 65535);

  platform->input(lx);
  platform->input(ly);
  result.append((lx->value() + 32768) * 255 / 65535);
  result.append((ly->value() + 32768) * 255 / 65535);

  return result;
}

auto DualShock::invalid(u8 data) -> u8 {
  debug(unusual, "[DualShock] Invalid command byte ", hex(data));
  _active = false;
  state = State::Idle;
  return 0xff;
}
