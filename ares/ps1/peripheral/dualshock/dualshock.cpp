// NOTE: A real Dual Shock starts up in Digital Mode
// However, we start up in Analog Mode due to there not being
// enough buttons to properly map the "Analog" button on the controller

DualShock::DualShock(Node::Port parent) {
  node = parent->append<Node::Peripheral>("DualShock");

  axis = node->append<Node::Input::Axis>("Axis");

  lx                          = node->append<Node::Input::Axis  >("L-Stick X");
  ly                          = node->append<Node::Input::Axis  >("L-Stick Y");
  rx                          = node->append<Node::Input::Axis  >("R-Stick X");
  ry                          = node->append<Node::Input::Axis  >("R-Stick Y");
  up                          = node->append<Node::Input::Button>("Up");
  down                        = node->append<Node::Input::Button>("Down");
  left                        = node->append<Node::Input::Button>("Left");
  right                       = node->append<Node::Input::Button>("Right");
  cross                       = node->append<Node::Input::Button>("Cross");
  circle                      = node->append<Node::Input::Button>("Circle");
  square                      = node->append<Node::Input::Button>("Square");
  triangle                    = node->append<Node::Input::Button>("Triangle");
  l1                          = node->append<Node::Input::Button>("L1");
  l2                          = node->append<Node::Input::Button>("L2");
  l3                          = node->append<Node::Input::Button>("L3");
  r1                          = node->append<Node::Input::Button>("R1");
  r2                          = node->append<Node::Input::Button>("R2");
  r3                          = node->append<Node::Input::Button>("R3");
  select                      = node->append<Node::Input::Button>("Select");
  start                       = node->append<Node::Input::Button>("Start");
  mode                        = node->append<Node::Input::Button>("Mode");
  rumble                      = node->append<Node::Input::Rumble>("Rumble");
  maxOutputReducer1LeftStick  = node->append<Node::Input::Button>("L-Stick Max Output Reducer 1");
  maxOutputReducer2LeftStick  = node->append<Node::Input::Button>("L-Stick Max Output Reducer 2");
  maxOutputReducer1RightStick = node->append<Node::Input::Button>("R-Stick Max Output Reducer 1");
  maxOutputReducer2RightStick = node->append<Node::Input::Button>("R-Stick Max Output Reducer 2");

  analogMode = 1;
  newRumbleMode = 0;
  configMode = 0;
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

  //old rumble mode
  if(!newRumbleMode && command == 0x42) {
    switch(commandStep) {
      case 1: inputData.reset(); inputData.append(input); break;
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

    //Global commands: these work during any operation mode
    switch(input) {
      case 0x42: outputData.append(readPad()); break;
      case 0x43: {
        if(configMode) outputData.append({0x00, 0x00, 0x00, 0x00, 0x00, 0x00});
        else outputData.append(readPad());
        break;
      default:
        if(configMode) {
          switch(input) {
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

        outputData.reset();
        output = invalid(input);
        break;
      }
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

  Node::Setting::String stickOutputStyle = axis->append<Node::Setting::String>("Stick Output Style", "Square (Maximum Virtual)", [&](auto value) {
    axis->setOutputStyle(value);
    });
  stickOutputStyle->setDynamic(true);
  stickOutputStyle->setAllowedValues({"Custom Octagon (Virtual)", "Custom Circle", "Custom Octagon (Morphed)", "Circle (Maximum)", "Square (Maximum Virtual)", "Square (Maximum Morphed)"});

  Node::Setting::Real stickMaxOutputReducerOneFactor = axis->append<Node::Setting::Real>("Stick Max Output Reducer 1 Factor", 0.50, [&](auto value) {
    axis->setMaxOutputReducerOneFactor(value);
    });
  stickMaxOutputReducerOneFactor->setDynamic(true);

  Node::Setting::Real stickMaxOutputReducerTwoFactor = axis->append<Node::Setting::Real>("Stick Max Output Reducer 2 Factor", 0.25, [&](auto value) {
    axis->setMaxOutputReducerTwoFactor(value);
    });
  stickMaxOutputReducerTwoFactor->setDynamic(true);

  Node::Setting::Real stickCustomMaxOutput = axis->append<Node::Setting::Real>("Stick Custom Max Output", 127.5, [&](auto value) {
    axis->setCustomMaxOutput(value);
    });
  stickCustomMaxOutput->setDynamic(true);

  Node::Setting::String stickDeadzoneShape = axis->append<Node::Setting::String>("Stick Deadzone Shape", "Axial", [&](auto value) {
    axis->setDeadzoneShape(value);
    });
  stickDeadzoneShape->setDynamic(true);
  stickDeadzoneShape->setAllowedValues({"Axial", "Radial"});

  Node::Setting::Real stickDeadzoneSize = axis->append<Node::Setting::Real>("Stick Deadzone Size", 6.0, [&](auto value) {
    axis->setDeadzoneSize(value);
    });
  stickDeadzoneSize->setDynamic(true);

  Node::Setting::Real stickProportionalSensitivity = axis->append<Node::Setting::Real>("Stick Proportional Sensitivity", 1.0, [&](auto value) {
    axis->setProportionalSensitivity(value);
    });
  stickProportionalSensitivity->setDynamic(true);

  Node::Setting::String stickResponseCurve = axis->append<Node::Setting::String>("Stick Response Curve", "Linear (Default)", [&](auto value) {
    axis->setResponseCurve(value);
    });
  stickResponseCurve->setDynamic(true);
  stickResponseCurve->setAllowedValues({"Linear (Default)", "Relaxed to Aggressive", "Aggressive to Relaxed", "Relaxed to Linear", "Linear to Relaxed", "Aggressive to Linear", "Linear to Aggressive"});


  Node::Setting::Real stickRangeNormalizedInflectionPoint = axis->append<Node::Setting::Real>("Stick Range Normalized Inflection Point", 0.5, [&](auto value) {
    axis->setRangeNormalizedInflectionPoint(value);
    });
  stickRangeNormalizedInflectionPoint->setDynamic(true);

  Node::Setting::Real stickResponseStrength = axis->append<Node::Setting::Real>("Stick Response Strength", 0.0, [&](auto value) {
    axis->setResponseStrength(value);
    });
  stickResponseStrength->setDynamic(true);

  Node::Setting::Boolean stickVirtualNotch = axis->append<Node::Setting::Boolean>("Enable Stick Virtual Notches", false, [&](auto value) {
    axis->virtualNotch();
    });
  stickVirtualNotch->setDynamic(true);

  Node::Setting::Real stickNotchLengthFromEdge = axis->append<Node::Setting::Real>("Stick Notch Length from Edge", 0.1, [&](auto value) {
    axis->setNotchLengthFromEdge(value);
    });
  stickNotchLengthFromEdge->setDynamic(true);

  Node::Setting::Real stickNotchAngularSnappingDistance = axis->append<Node::Setting::Real>("Stick Notch Angular Snapping Distance", 0.0, [&](auto value) {
    axis->setNotchAngularSnappingDistance(value);
    });
  stickNotchAngularSnappingDistance->setDynamic(true);

  platform->input(rx);
  platform->input(ry);
  platform->input(maxOutputReducer1RightStick);
  platform->input(maxOutputReducer2RightStick);

  string configuredOutputStyleChoiceRightStick = axis->outputStyle();
  if(configuredOutputStyleChoiceRightStick == "Custom Octagon (Virtual)") outputStyleRightStick = OutputStyleRightStick::CustomVirtualOctagon; //actually a square
  if(configuredOutputStyleChoiceRightStick == "Custom Circle") outputStyleRightStick = OutputStyleRightStick::CustomCircle;
  if(configuredOutputStyleChoiceRightStick == "Custom Octagon (Morphed)") outputStyleRightStick = OutputStyleRightStick::CustomMorphedOctagon; //actually a square
  if(configuredOutputStyleChoiceRightStick == "Circle (Maximum)") outputStyleRightStick = OutputStyleRightStick::MaxCircle;
  if(configuredOutputStyleChoiceRightStick == "Square (Maximum Virtual)") outputStyleRightStick = OutputStyleRightStick::MaxVirtualSquare;
  if(configuredOutputStyleChoiceRightStick == "Square (Maximum Morphed)") outputStyleRightStick = OutputStyleRightStick::MaxMorphedSquare;
  if(configuredOutputStyleChoiceRightStick == "Circle (Diagonal)" || configuredOutputStyleChoiceRightStick == "Octagon (Virtual) (Default)" || configuredOutputStyleChoiceRightStick == "Circle (Cardinal)" || configuredOutputStyleChoiceRightStick == "Octagon (Morphed)") outputStyleRightStick = OutputStyleRightStick::MaxVirtualSquare; //temporary guard

  auto cardinalMaxRightStick = 127.5;
  auto diagonalMaxRightStick = 127.5;
  auto innerDeadzoneRightStick = 6.0; //missing information on deadzone for DualShock potentiometers; use arbitrary number roughly 4.7% of cardinalMax instead (common software default in gaming industry seems to be 5.0%) 
  auto saturationRadiusRightStick = (innerDeadzoneRightStick + diagonalMaxRightStick + sqrt(pow(innerDeadzoneRightStick + diagonalMaxRightStick, 2.0) - 2.0 * sqrt(2.0) * diagonalMaxRightStick * innerDeadzoneRightStick)) / sqrt(2.0); //from linear response curve function within axis->processDeadzoneAndResponseCurve, substitute saturationRadius * sqrt(2) / 2 for right-hand lengthAbsolute and set diagonalMax as the result then solve for saturationRadius
  auto offsetRightStick = -0.5;

  string configuredDeadzoneShapeRightStick = axis->deadzoneShape();
  auto configuredInnerDeadzoneRightStick = max(0.0, axis->deadzoneSize()); //user-defined [0, cardinalMax) (default 7.0); deadzone where input less than assigned value is 0
  auto configuredCustomMaxOutputRightStick = max(0.0, axis->customMaxOutput()); //user-defined [0, 127] (default 85.0); only affects outputStyleChoice strings that include "Custom"
  auto customMaxOutputMultiplierRightStick = configuredCustomMaxOutputRightStick / cardinalMaxRightStick; //for CustomOctagon, customMaxOutput divided by its outerRadiusDeadzoneMax could be done instead but the current works fine because of the later clamps

  auto maxOutputReduction1RightStick = (maxOutputReducer1RightStick->value()) ? max(0.0, axis->maxOutputReducerOneFactor()) : 0.0; //user-defined (default 0.50); same questions as above; currently tied to either a button or key hold for activation; Is this best, or is switching to a toggle, cycle, or some combination better?
  auto maxOutputReduction2RightStick = (maxOutputReducer2RightStick->value()) ? max(0.0, axis->maxOutputReducerTwoFactor()) : 0.0; //user-defined (default 0.25); refer to nearest above comment
  auto maxOutputReductionRightStick = max(0.0, 1.0 - (maxOutputReduction1RightStick + maxOutputReduction2RightStick));

  switch(outputStyleRightStick) { //too messy? single-line cases seemed worse and if-else statements seemed hard to follow

  case OutputStyleRightStick::CustomMorphedOctagon:
    diagonalMaxRightStick = maxOutputReductionRightStick * customMaxOutputMultiplierRightStick * 127.5;
    cardinalMaxRightStick = maxOutputReductionRightStick * customMaxOutputMultiplierRightStick * 127.5;
    saturationRadiusRightStick = maxOutputReductionRightStick * customMaxOutputMultiplierRightStick * 127.5;
    break;
  case OutputStyleRightStick::CustomVirtualOctagon:
    diagonalMaxRightStick = maxOutputReductionRightStick * customMaxOutputMultiplierRightStick * 127.5;
    cardinalMaxRightStick = maxOutputReductionRightStick * customMaxOutputMultiplierRightStick * 127.5;
    saturationRadiusRightStick = (configuredInnerDeadzoneRightStick + diagonalMaxRightStick + sqrt(pow(configuredInnerDeadzoneRightStick + diagonalMaxRightStick, 2.0) - 2.0 * sqrt(2.0) * diagonalMaxRightStick * configuredInnerDeadzoneRightStick)) / sqrt(2.0);
    break;
  case OutputStyleRightStick::CustomCircle:
    cardinalMaxRightStick = maxOutputReductionRightStick * customMaxOutputMultiplierRightStick * 127.5;
    saturationRadiusRightStick = maxOutputReductionRightStick * customMaxOutputMultiplierRightStick * 127.5;
    break;
  case OutputStyleRightStick::MaxCircle:
    cardinalMaxRightStick = maxOutputReductionRightStick * 127.5;
    saturationRadiusRightStick = maxOutputReductionRightStick * 127.5;
    break;
  case OutputStyleRightStick::MaxVirtualSquare:
    diagonalMaxRightStick = maxOutputReductionRightStick * 127.5;
    cardinalMaxRightStick = maxOutputReductionRightStick * 127.5;
    saturationRadiusRightStick = (configuredInnerDeadzoneRightStick + diagonalMaxRightStick + sqrt(pow(configuredInnerDeadzoneRightStick + diagonalMaxRightStick, 2.0) - 2.0 * sqrt(2.0) * diagonalMaxRightStick * configuredInnerDeadzoneRightStick)) / sqrt(2.0);
    break;
  case OutputStyleRightStick::MaxMorphedSquare:
    diagonalMaxRightStick = maxOutputReductionRightStick * 127.5;
    cardinalMaxRightStick = maxOutputReductionRightStick * 127.5;
    saturationRadiusRightStick = maxOutputReductionRightStick * 127.5;
    break;
  }

  auto configuredNotchLengthFromEdgeRightStick = axis->notchLengthFromEdge(); //user-defined [0.0, 1.0] (default 0.1); the default value cannot be configured by user in other projects
  auto configuredMaxNotchAngularDistRightStick = axis->notchAngularSnappingDistance(); //user-defined in degrees [0.0, 45.0] (default 0.0); values under 15.0 are likely to be more favorable
  auto configuredVirtualNotchRightStick = axis->virtualNotch();

  string configuredResponseCurveModeRightStick = axis->responseCurve();
  if(configuredResponseCurveModeRightStick == "Linear (Default)") responseRightStick = ResponseRightStick::Linear;
  if(configuredResponseCurveModeRightStick == "Relaxed to Aggressive") responseRightStick = ResponseRightStick::RelaxedToAggressive;
  if(configuredResponseCurveModeRightStick == "Aggressive to Relaxed") responseRightStick = ResponseRightStick::AggressiveToRelaxed;
  if(configuredResponseCurveModeRightStick == "Relaxed to Linear") responseRightStick = ResponseRightStick::RelaxedToLinear;
  if(configuredResponseCurveModeRightStick == "Linear to Relaxed") responseRightStick = ResponseRightStick::LinearToRelaxed;
  if(configuredResponseCurveModeRightStick == "Aggressive to Linear") responseRightStick = ResponseRightStick::AggressiveToLinear;
  if(configuredResponseCurveModeRightStick == "Linear to Aggressive") responseRightStick = ResponseRightStick::LinearToAggressive;

  auto configuredRangeNormalizedInflectionPointRightStick = std::clamp(axis->rangeNormalizedInflectionPoint(), 0.005, 0.995); //user-defined configuredInnerDeadzone, cardinalMax) (default 50.0)
  auto configuredResponseStrengthRightStick = axis->responseStrength(); //user-defined (0.0, 100.0] (default 100.0); used to produce a more relaxed or aggressive curve; values close to 0.0 and 100.0 create strongest response when relaxed and aggressive, respectively
  auto configuredProportionalSensitivityRightStick = axis->proportionalSensitivity(); //user-defined (default 1.0); Should this only apply to a linear response? What percentage range should be used? Place outside of Gamepad::responseCurve()?

  //scale {-32767 ... +32767} to {-saturationRadius + offset ... +saturationRadius + offset}
  auto arx = axis->setOperatingRange(rx->value(), saturationRadiusRightStick, offsetRightStick);
  auto ary = axis->setOperatingRange(ry->value(), saturationRadiusRightStick, offsetRightStick);

  if(configuredVirtualNotchRightStick == true) {
    auto initialLengthRightStick = hypot(arx - offsetRightStick, ary - offsetRightStick);
    auto initialAngleRightStick = atan2(ary - offsetRightStick, arx - offsetRightStick);
    auto currentAngleRightStick = axis->virtualNotch(initialLengthRightStick, initialAngleRightStick, saturationRadiusRightStick, configuredNotchLengthFromEdgeRightStick, configuredMaxNotchAngularDistRightStick);
    arx = cos(currentAngleRightStick) * initialLengthRightStick;
    ary = sin(currentAngleRightStick) * initialLengthRightStick;
  }

  //create inner dead-zone of chosen shape in range {-configuredInnerDeadzoneRightStick ... +configuredInnerDeadzoneRightStick} and scale from it up to saturationRadius
  if(configuredDeadzoneShapeRightStick == "Axial") {
    auto lengthAbsoluteRX = abs(arx - offsetRightStick);
    auto lengthAbsoluteRY = abs(ary - offsetRightStick);
    arx = axis->processDeadzoneAndResponseCurve(arx, lengthAbsoluteRX, configuredInnerDeadzoneRightStick, saturationRadiusRightStick, offsetRightStick, configuredRangeNormalizedInflectionPointRightStick, configuredResponseStrengthRightStick, configuredProportionalSensitivityRightStick, configuredResponseCurveModeRightStick);
    ary = axis->processDeadzoneAndResponseCurve(ary, lengthAbsoluteRY, configuredInnerDeadzoneRightStick, saturationRadiusRightStick, offsetRightStick, configuredRangeNormalizedInflectionPointRightStick, configuredResponseStrengthRightStick, configuredProportionalSensitivityRightStick, configuredResponseCurveModeRightStick);
  } else {
    auto lengthRightStick = hypot(arx - offsetRightStick, ary - offsetRightStick);
    arx = axis->processDeadzoneAndResponseCurve(arx, lengthRightStick, configuredInnerDeadzoneRightStick, saturationRadiusRightStick, offsetRightStick, configuredRangeNormalizedInflectionPointRightStick, configuredResponseStrengthRightStick, configuredProportionalSensitivityRightStick, configuredResponseCurveModeRightStick);
    ary = axis->processDeadzoneAndResponseCurve(ary, lengthRightStick, configuredInnerDeadzoneRightStick, saturationRadiusRightStick, offsetRightStick, configuredRangeNormalizedInflectionPointRightStick, configuredResponseStrengthRightStick, configuredProportionalSensitivityRightStick, configuredResponseCurveModeRightStick);
  }

  auto scaledLengthRightStick = hypot(arx - offsetRightStick, ary - offsetRightStick);
  if(scaledLengthRightStick > saturationRadiusRightStick) {
    scaledLengthRightStick = saturationRadiusRightStick / scaledLengthRightStick;
    arx = axis->revisePosition(arx, scaledLengthRightStick, offsetRightStick);
    ary = axis->revisePosition(ary, scaledLengthRightStick, offsetRightStick);
  }

  //let cardinalMax and diagonalMax define boundaries and restrict to a square gate
  if(outputStyleRightStick == OutputStyleRightStick::CustomVirtualOctagon || outputStyleRightStick == OutputStyleRightStick::CustomMorphedOctagon || outputStyleRightStick == OutputStyleRightStick::MaxVirtualSquare || outputStyleRightStick == OutputStyleRightStick::MaxMorphedSquare) {
    double edgeRX = 0.0;
    double edgeRY = 0.0;

    axis->applyGateBoundaries(cardinalMaxRightStick, diagonalMaxRightStick, arx, ary, offsetRightStick, edgeRX, edgeRY);

    auto distanceToEdgeRightStick = hypot(edgeRX, edgeRY);

    if(outputStyleRightStick == OutputStyleRightStick::CustomVirtualOctagon || outputStyleRightStick == OutputStyleRightStick::MaxVirtualSquare) {
      auto currentLengthRightStick = hypot(arx - offsetRightStick, ary - offsetRightStick);
      if(currentLengthRightStick > distanceToEdgeRightStick) {
        arx = edgeRX;
        ary = edgeRY;
      }
    } else {
      auto scaleRightStick = distanceToEdgeRightStick / saturationRadiusRightStick;
      arx = axis->revisePosition(arx, scaleRightStick, offsetRightStick);
      ary = axis->revisePosition(ary, scaleRightStick, offsetRightStick);
    }
  }

  //keep cardinal input within positive and negative bounds of cardinalMax
  if(outputStyleRightStick == OutputStyleRightStick::CustomVirtualOctagon || outputStyleRightStick == OutputStyleRightStick::MaxVirtualSquare) {
    arx = axis->clampAxisToNearestBoundary(arx, offsetRightStick, cardinalMaxRightStick);
    ary = axis->clampAxisToNearestBoundary(ary, offsetRightStick, cardinalMaxRightStick);
  }

  //add epsilon to counteract floating point precision error
  arx = axis->counteractPrecisionError(arx);
  ary = axis->counteractPrecisionError(ary);

  result.append(u8(arx + 128.0));
  result.append(u8(ary + 128.0));

  platform->input(lx);
  platform->input(ly);
  platform->input(maxOutputReducer1LeftStick);
  platform->input(maxOutputReducer2LeftStick);

  string configuredOutputStyleChoiceLeftStick = axis->outputStyle();
  if(configuredOutputStyleChoiceLeftStick == "Custom Octagon (Virtual)") outputStyleLeftStick = OutputStyleLeftStick::CustomVirtualOctagon;
  if(configuredOutputStyleChoiceLeftStick == "Custom Circle") outputStyleLeftStick = OutputStyleLeftStick::CustomCircle;
  if(configuredOutputStyleChoiceLeftStick == "Custom Octagon (Morphed)") outputStyleLeftStick = OutputStyleLeftStick::CustomMorphedOctagon;
  if(configuredOutputStyleChoiceLeftStick == "Circle (Maximum)") outputStyleLeftStick = OutputStyleLeftStick::MaxCircle;
  if(configuredOutputStyleChoiceLeftStick == "Square (Maximum Virtual)") outputStyleLeftStick = OutputStyleLeftStick::MaxVirtualSquare;
  if(configuredOutputStyleChoiceLeftStick == "Square (Maximum Morphed)") outputStyleLeftStick = OutputStyleLeftStick::MaxMorphedSquare;
  if(configuredOutputStyleChoiceLeftStick == "Circle (Diagonal)" || configuredOutputStyleChoiceLeftStick == "Octagon (Virtual) (Default)" || configuredOutputStyleChoiceLeftStick == "Circle (Cardinal)" || configuredOutputStyleChoiceLeftStick == "Octagon (Morphed)") outputStyleLeftStick = OutputStyleLeftStick::MaxVirtualSquare; //temporary guard

  auto cardinalMaxLeftStick = 127.5;
  auto diagonalMaxLeftStick = 127.5;
  auto innerDeadzoneLeftStick = 6.0; //missing information on deadzone for DualShock potentiometers; use arbitrary number roughly 4.7% of cardinalMax instead (common software default in gaming industry seems to be 5.0%) 
  auto saturationRadiusLeftStick = (innerDeadzoneLeftStick + diagonalMaxLeftStick + sqrt(pow(innerDeadzoneLeftStick + diagonalMaxLeftStick, 2.0) - 2.0 * sqrt(2.0) * diagonalMaxLeftStick * innerDeadzoneLeftStick)) / sqrt(2.0); //from linear response curve function within axis->processDeadzoneAndResponseCurve, substitute saturationRadius * sqrt(2) / 2 for right-hand lengthAbsolute and set diagonalMax as the result then solve for saturationRadius
  auto offsetLeftStick = -0.5;

  string configuredDeadzoneShapeLeftStick = axis->deadzoneShape();
  auto configuredInnerDeadzoneLeftStick = max(0.0, axis->deadzoneSize()); //user-defined [0, cardinalMax) (default 7.0); deadzone where input less than assigned value is 0
  auto configuredCustomMaxOutputLeftStick = max(0.0, axis->customMaxOutput()); //user-defined [0, 127] (default 85.0); only affects outputStyleChoice strings that include "Custom"
  auto customMaxOutputMultiplierLeftStick = configuredCustomMaxOutputLeftStick / cardinalMaxLeftStick; //for CustomOctagon, customMaxOutput divided by its outerRadiusDeadzoneMax could be done instead but the current works fine because of the later clamps

  auto maxOutputReduction1LeftStick = (maxOutputReducer1LeftStick->value()) ? max(0.0, axis->maxOutputReducerOneFactor()) : 0.0; //user-defined (default 0.50); same questions as above; currently tied to either a button or key hold for activation; Is this best, or is switching to a toggle, cycle, or some combination better?
  auto maxOutputReduction2LeftStick = (maxOutputReducer2LeftStick->value()) ? max(0.0, axis->maxOutputReducerTwoFactor()) : 0.0; //user-defined (default 0.25); refer to nearest above comment
  auto maxOutputReductionLeftStick = max(0.0, 1.0 - (maxOutputReduction1LeftStick + maxOutputReduction2LeftStick));

  switch(outputStyleLeftStick) { //too messy? single-line cases seemed worse and if-else statements seemed hard to follow

  case OutputStyleLeftStick::CustomMorphedOctagon:
    diagonalMaxLeftStick = maxOutputReductionLeftStick * customMaxOutputMultiplierLeftStick * 127.5;
    cardinalMaxLeftStick = maxOutputReductionLeftStick * customMaxOutputMultiplierLeftStick * 127.5;
    saturationRadiusLeftStick = maxOutputReductionLeftStick * customMaxOutputMultiplierLeftStick * 127.5;
    break;
  case OutputStyleLeftStick::CustomVirtualOctagon:
    diagonalMaxLeftStick = maxOutputReductionLeftStick * customMaxOutputMultiplierLeftStick * 127.5;
    cardinalMaxLeftStick = maxOutputReductionLeftStick * customMaxOutputMultiplierLeftStick * 127.5;
    saturationRadiusLeftStick = (configuredInnerDeadzoneLeftStick + diagonalMaxLeftStick + sqrt(pow(configuredInnerDeadzoneLeftStick + diagonalMaxLeftStick, 2.0) - 2.0 * sqrt(2.0) * diagonalMaxLeftStick * configuredInnerDeadzoneLeftStick)) / sqrt(2.0);
    break;
  case OutputStyleLeftStick::CustomCircle:
    cardinalMaxLeftStick = maxOutputReductionLeftStick * customMaxOutputMultiplierLeftStick * 127.5;
    saturationRadiusLeftStick = maxOutputReductionLeftStick * customMaxOutputMultiplierLeftStick * 127.5;
    break;
  case OutputStyleLeftStick::MaxCircle:
    cardinalMaxLeftStick = maxOutputReductionLeftStick * 127.5;
    saturationRadiusLeftStick = maxOutputReductionLeftStick * 127.5;
    break;
  case OutputStyleLeftStick::MaxVirtualSquare:
    diagonalMaxLeftStick = maxOutputReductionLeftStick * 127.5;
    cardinalMaxLeftStick = maxOutputReductionLeftStick * 127.5;
    saturationRadiusLeftStick = (configuredInnerDeadzoneLeftStick + diagonalMaxLeftStick + sqrt(pow(configuredInnerDeadzoneLeftStick + diagonalMaxLeftStick, 2.0) - 2.0 * sqrt(2.0) * diagonalMaxLeftStick * configuredInnerDeadzoneLeftStick)) / sqrt(2.0);
    break;
  case OutputStyleLeftStick::MaxMorphedSquare:
    diagonalMaxLeftStick = maxOutputReductionLeftStick * 127.5;
    cardinalMaxLeftStick = maxOutputReductionLeftStick * 127.5;
    saturationRadiusLeftStick = maxOutputReductionLeftStick * 127.5;
    break;
  }

  auto configuredNotchLengthFromEdgeLeftStick = axis->notchLengthFromEdge(); //user-defined [0.0, 1.0] (default 0.1); the default value cannot be configured by user in other projects
  auto configuredMaxNotchAngularDistLeftStick = axis->notchAngularSnappingDistance(); //user-defined in degrees [0.0, 45.0] (default 0.0); values under 15.0 are likely to be more favorable
  auto configuredVirtualNotchLeftStick = axis->virtualNotch();

  string configuredResponseCurveModeLeftStick = axis->responseCurve();
  if(configuredResponseCurveModeLeftStick == "Linear (Default)") responseLeftStick = ResponseLeftStick::Linear;
  if(configuredResponseCurveModeLeftStick == "Relaxed to Aggressive") responseLeftStick = ResponseLeftStick::RelaxedToAggressive;
  if(configuredResponseCurveModeLeftStick == "Aggressive to Relaxed") responseLeftStick = ResponseLeftStick::AggressiveToRelaxed;
  if(configuredResponseCurveModeLeftStick == "Relaxed to Linear") responseLeftStick = ResponseLeftStick::RelaxedToLinear;
  if(configuredResponseCurveModeLeftStick == "Linear to Relaxed") responseLeftStick = ResponseLeftStick::LinearToRelaxed;
  if(configuredResponseCurveModeLeftStick == "Aggressive to Linear") responseLeftStick = ResponseLeftStick::AggressiveToLinear;
  if(configuredResponseCurveModeLeftStick == "Linear to Aggressive") responseLeftStick = ResponseLeftStick::LinearToAggressive;

  auto configuredRangeNormalizedInflectionPointLeftStick = std::clamp(axis->rangeNormalizedInflectionPoint(), 0.005, 0.995); //user-defined (configuredInnerDeadzone, cardinalMax) (default 50.0)
  auto configuredResponseStrengthLeftStick = axis->responseStrength(); //user-defined (0.0, 100.0] (default 100.0); used to produce a more relaxed or aggressive curve; values close to 0.0 and 100.0 create strongest response when relaxed and aggressive, respectively
  auto configuredProportionalSensitivityLeftStick = axis->proportionalSensitivity(); //user-defined (default 1.0); Should this only apply to a linear response? What percentage range should be used? Place outside of Gamepad::responseCurve()?

  //scale {-32767 ... +32767} to {-saturationRadius + offset ... +saturationRadius + offset}
  auto alx = axis->setOperatingRange(lx->value(), saturationRadiusLeftStick, offsetLeftStick);
  auto aly = axis->setOperatingRange(ly->value(), saturationRadiusLeftStick, offsetLeftStick);

  if(configuredVirtualNotchLeftStick == true) {
    auto initialLengthLeftStick = hypot(alx - offsetLeftStick, aly - offsetLeftStick);
    auto initialAngleLeftStick = atan2(aly - offsetLeftStick, alx - offsetLeftStick);
    auto currentAngleLeftStick = axis->virtualNotch(initialLengthLeftStick, initialAngleLeftStick, saturationRadiusLeftStick, configuredNotchLengthFromEdgeLeftStick, configuredMaxNotchAngularDistLeftStick);
    alx = cos(currentAngleLeftStick) * initialLengthLeftStick;
    aly = sin(currentAngleLeftStick) * initialLengthLeftStick;
  }

  //create axial dead-zone of chosen shape in range {-configuredInnerDeadzoneLeftStick ... +configuredInnerDeadzoneLeftStick} and scale from it up to saturationRadius
  if(configuredDeadzoneShapeLeftStick == "Axial") {
    auto lengthAbsoluteLX = abs(alx - offsetLeftStick);
    auto lengthAbsoluteLY = abs(aly - offsetLeftStick);
    alx = axis->processDeadzoneAndResponseCurve(alx, lengthAbsoluteLX, configuredInnerDeadzoneLeftStick, saturationRadiusLeftStick, offsetLeftStick, configuredRangeNormalizedInflectionPointLeftStick, configuredResponseStrengthLeftStick, configuredProportionalSensitivityLeftStick, configuredResponseCurveModeLeftStick);
    aly = axis->processDeadzoneAndResponseCurve(aly, lengthAbsoluteLY, configuredInnerDeadzoneLeftStick, saturationRadiusLeftStick, offsetLeftStick, configuredRangeNormalizedInflectionPointLeftStick, configuredResponseStrengthLeftStick, configuredProportionalSensitivityLeftStick, configuredResponseCurveModeLeftStick);
  } else {
    auto lengthLeftStick = hypot(alx - offsetLeftStick, aly - offsetLeftStick);
    alx = axis->processDeadzoneAndResponseCurve(alx, lengthLeftStick, configuredInnerDeadzoneLeftStick, saturationRadiusLeftStick, offsetLeftStick, configuredRangeNormalizedInflectionPointLeftStick, configuredResponseStrengthLeftStick, configuredProportionalSensitivityLeftStick, configuredResponseCurveModeLeftStick);
    aly = axis->processDeadzoneAndResponseCurve(aly, lengthLeftStick, configuredInnerDeadzoneLeftStick, saturationRadiusLeftStick, offsetLeftStick, configuredRangeNormalizedInflectionPointLeftStick, configuredResponseStrengthLeftStick, configuredProportionalSensitivityLeftStick, configuredResponseCurveModeLeftStick);
  }

  auto scaledLengthLeftStick = hypot(alx - offsetLeftStick, aly - offsetLeftStick);
  if(scaledLengthLeftStick > saturationRadiusLeftStick) {
    scaledLengthLeftStick = saturationRadiusLeftStick / scaledLengthLeftStick;
    alx = axis->revisePosition(alx, scaledLengthLeftStick, offsetLeftStick);
    aly = axis->revisePosition(aly, scaledLengthLeftStick, offsetLeftStick);
  }

  //let cardinalMax and diagonalMax define boundaries and restrict to a square gate
  if(outputStyleLeftStick == OutputStyleLeftStick::CustomVirtualOctagon || outputStyleLeftStick == OutputStyleLeftStick::CustomMorphedOctagon || outputStyleLeftStick == OutputStyleLeftStick::MaxVirtualSquare || outputStyleLeftStick == OutputStyleLeftStick::MaxMorphedSquare) {
    double edgeLX = 0.0;
    double edgeLY = 0.0;

    axis->applyGateBoundaries(cardinalMaxLeftStick, diagonalMaxLeftStick, alx, aly, offsetLeftStick, edgeLX, edgeLY);

    auto distanceToEdgeLeftStick = hypot(edgeLX, edgeLY);

    if(outputStyleLeftStick == OutputStyleLeftStick::CustomVirtualOctagon || outputStyleLeftStick == OutputStyleLeftStick::MaxVirtualSquare) {
      auto currentLengthLeftStick = hypot(alx - offsetLeftStick, aly - offsetLeftStick);
      if(currentLengthLeftStick > distanceToEdgeLeftStick) {
        alx = edgeLX;
        aly = edgeLY;
      }
    } else {
      auto scaleLeftStick = distanceToEdgeLeftStick / saturationRadiusLeftStick;
      alx = axis->revisePosition(alx, scaleLeftStick, offsetLeftStick);
      aly = axis->revisePosition(aly, scaleLeftStick, offsetLeftStick);
    }
  }

  //keep cardinal input within positive and negative bounds of cardinalMax
  if(outputStyleLeftStick == OutputStyleLeftStick::CustomVirtualOctagon || outputStyleLeftStick == OutputStyleLeftStick::MaxVirtualSquare) {
    alx = axis->clampAxisToNearestBoundary(alx, offsetLeftStick, cardinalMaxLeftStick);
    aly = axis->clampAxisToNearestBoundary(aly, offsetLeftStick, cardinalMaxLeftStick);
  }

  alx = axis->counteractPrecisionError(alx);
  aly = axis->counteractPrecisionError(aly);

  result.append(u8(alx + 128.0));
  result.append(u8(aly + 128.0));

  return result;
}

auto DualShock::invalid(u8 data) -> u8 {
  debug(unusual, "[DualShock] Invalid command byte ", hex(data));
  _active = false;
  state = State::Idle;
  return 0xff;
}
