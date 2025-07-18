Aleck64Controls::Aleck64Controls(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Aleck64");

  //Detect which port we are connected to and map the right arcade controls accordingly
  if(parent->name() == "Controller Port 1") {
    playerIndex = 1;
  } else if(parent->name() == "Controller Port 2") {
    playerIndex = 2;
  }
}

Aleck64Controls::~Aleck64Controls() {

}

auto Aleck64Controls::comm(n8 send, n8 recv, n8 input[], n8 output[]) -> n2 {
  b1 valid = 0;
  b1 over = 0;

  //status
  if(input[0] == 0x00 || input[0] == 0xff) {
    output[0] = 0x05;  //0x05 = gamepad; 0x02 = mouse
    output[1] = 0x00;
    output[2] = 0x02;  //0x02 = nothing present in controller slot
    valid = 1;
  }

  //read controller state
  if(input[0] == 0x01) {
    u32 data = read();
    output[0] = data >> 24;
    output[1] = data >> 16;
    output[2] = data >>  8;
    output[3] = data >>  0;
    if(recv <= 4) {
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

auto Aleck64Controls::read() -> n32 {
  aleck64.controls.poll();

  Node::Setting::String stickOutputStyle = axis->append<Node::Setting::String>("Stick Output Style", "Octagon (Virtual) (Default)", [&](auto value) {
    axis->setOutputStyle(value);
    });
  stickOutputStyle->setDynamic(true);
  stickOutputStyle->setAllowedValues({"Custom Octagon (Virtual)", "Custom Circle", "Custom Octagon (Morphed)", "Circle (Diagonal)", "Octagon (Virtual) (Default)", "Circle (Maximum)", "Circle (Cardinal)", "Octagon (Morphed)", "Square (Maximum Virtual)", "Square (Maximum Morphed)"});

  Node::Setting::Real stickMaxOutputReducerOneFactor = axis->append<Node::Setting::Real>("Stick Max Output Reducer 1 Factor", 0.50, [&](auto value) {
    axis->setMaxOutputReducerOneFactor(value);
    });
  stickMaxOutputReducerOneFactor->setDynamic(true);

  Node::Setting::Real stickMaxOutputReducerTwoFactor = axis->append<Node::Setting::Real>("Stick Max Output Reducer 2 Factor", 0.25, [&](auto value) {
    axis->setMaxOutputReducerTwoFactor(value);
    });
  stickMaxOutputReducerTwoFactor->setDynamic(true);

  Node::Setting::Real stickCustomMaxOutput = axis->append<Node::Setting::Real>("Stick Custom Max Output", 85.0, [&](auto value) {
    axis->setCustomMaxOutput(value);
    });
  stickCustomMaxOutput->setDynamic(true);

  Node::Setting::String stickDeadzoneShape = axis->append<Node::Setting::String>("Stick Deadzone Shape", "Axial", [&](auto value) {
    axis->setDeadzoneShape(value);
    });
  stickDeadzoneShape->setDynamic(true);
  stickDeadzoneShape->setAllowedValues({"Axial", "Radial"});

  Node::Setting::Real stickDeadzoneSize = axis->append<Node::Setting::Real>("Stick Deadzone Size", 7.0, [&](auto value) {
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

  string configuredOutputStyleChoice = axis->outputStyle();
  if(configuredOutputStyleChoice == "Custom Octagon (Virtual)") outputStyle = OutputStyle::CustomVirtualOctagon;
  if(configuredOutputStyleChoice == "Custom Circle") outputStyle = OutputStyle::CustomCircle;
  if(configuredOutputStyleChoice == "Custom Octagon (Morphed)") outputStyle = OutputStyle::CustomMorphedOctagon;
  if(configuredOutputStyleChoice == "Circle (Diagonal)") outputStyle = OutputStyle::DiagonalCircle;
  if(configuredOutputStyleChoice == "Octagon (Virtual) (Default)") outputStyle = OutputStyle::VirtualOctagon;
  if(configuredOutputStyleChoice == "Circle (Maximum)") outputStyle = OutputStyle::MaxCircle;
  if(configuredOutputStyleChoice == "Circle (Cardinal)") outputStyle = OutputStyle::CardinalCircle;
  if(configuredOutputStyleChoice == "Octagon (Morphed)") outputStyle = OutputStyle::MorphedOctagon;
  if(configuredOutputStyleChoice == "Square (Maximum Virtual)") outputStyle = OutputStyle::MaxVirtualSquare;
  if(configuredOutputStyleChoice == "Square (Maximum Morphed)") outputStyle = OutputStyle::MaxMorphedSquare;

  auto cardinalMax = 85.0;
  auto diagonalMax = 69.0;
  auto innerDeadzone = 7.0; // default should remain 7 (~8.2% of 85) as the deadzone is axial in nature and fights cardinalMax
  auto saturationRadius = (innerDeadzone + diagonalMax + sqrt(pow(innerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * innerDeadzone)) / sqrt(2.0); //from linear response curve function within axis->processDeadzoneAndResponseCurve, substitute saturationRadius * sqrt(2) / 2 for right-hand lengthAbsolute and set diagonalMax as the result then solve for saturationRadius
  auto offset = 0.0;

  string configuredDeadzoneShape = axis->deadzoneShape(); //user-defined Axial or Radial
  auto configuredInnerDeadzone = max(0.0, axis->deadzoneSize()); //user-defined [0, cardinalMax) (default 7.0); deadzone where input less than assigned value is 0
  auto configuredCustomMaxOutput = max(0.0, axis->customMaxOutput()); //user-defined [0, 127] (default 85.0); only affects outputStyleChoice strings that include "Custom"
  auto customMaxOutputMultiplier = configuredCustomMaxOutput / cardinalMax; //for CustomOctagon, customMaxOutput divided by its saturationRadius could be done instead but the current works fine because of the later clamps
  
  auto maxOutputReduction1 = aleck64.controls.controllerButton(playerIndex, "Max Output Reducer 1") ? max(0.0, axis->maxOutputReducerOneFactor()) : 0.0; //user-defined (default 0.50); same questions as above; currently tied to either a button or key hold for activation; Is this best, or is switching to a toggle, cycle, or some combination better?
  auto maxOutputReduction2 = aleck64.controls.controllerButton(playerIndex, "Max Output Reducer 2") ? max(0.0, axis->maxOutputReducerTwoFactor()) : 0.0; //user-defined (default 0.25); refer to nearest above comment
  auto maxOutputReduction = max(0.0, 1.0 - (maxOutputReduction1 + maxOutputReduction2));

  switch(outputStyle) { //too messy? single-line cases seemed worse and if-else statements seemed hard to follow

  case OutputStyle::CustomMorphedOctagon:
    diagonalMax = maxOutputReduction * customMaxOutputMultiplier * 69.0;
    cardinalMax = maxOutputReduction * customMaxOutputMultiplier * 85.0;
    saturationRadius = maxOutputReduction * customMaxOutputMultiplier * 85.0;
    break;
  case OutputStyle::CustomVirtualOctagon:
    diagonalMax = maxOutputReduction * customMaxOutputMultiplier * 69.0;
    cardinalMax = maxOutputReduction * customMaxOutputMultiplier * 85.0;
    saturationRadius = (configuredInnerDeadzone + diagonalMax + sqrt(pow(configuredInnerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * configuredInnerDeadzone)) / sqrt(2.0);
    break;
  case OutputStyle::CustomCircle:
    cardinalMax = maxOutputReduction * customMaxOutputMultiplier * 85.0;
    saturationRadius = maxOutputReduction * customMaxOutputMultiplier * 85.0;
    break;
  case OutputStyle::VirtualOctagon:
    diagonalMax = maxOutputReduction * 69.0;
    cardinalMax = maxOutputReduction * 85.0;
    saturationRadius = (configuredInnerDeadzone + diagonalMax + sqrt(pow(configuredInnerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * configuredInnerDeadzone)) / sqrt(2.0);
    break;
  case OutputStyle::DiagonalCircle:
    diagonalMax = maxOutputReduction * 69.0;
    saturationRadius = (configuredInnerDeadzone + diagonalMax + sqrt(pow(configuredInnerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * configuredInnerDeadzone)) / sqrt(2.0);
    cardinalMax = saturationRadius;
    break;
  case OutputStyle::MaxCircle:
    cardinalMax = maxOutputReduction * 127.0;
    saturationRadius = maxOutputReduction * 127.0;
    break;
  case OutputStyle::CardinalCircle:
    cardinalMax = maxOutputReduction * 85.0;
    saturationRadius = maxOutputReduction * 85.0;
    break;
  case OutputStyle::MorphedOctagon:
    diagonalMax = maxOutputReduction * 69.0;
    cardinalMax = maxOutputReduction * 85.0;
    saturationRadius = maxOutputReduction * 85.0;
    break;
  case OutputStyle::MaxVirtualSquare:
    diagonalMax = maxOutputReduction * 127.0;
    cardinalMax = maxOutputReduction * 127.0;
    saturationRadius = (configuredInnerDeadzone + diagonalMax + sqrt(pow(configuredInnerDeadzone + diagonalMax, 2.0) - 2.0 * sqrt(2.0) * diagonalMax * configuredInnerDeadzone)) / sqrt(2.0);
    break;
  case OutputStyle::MaxMorphedSquare:
    diagonalMax = maxOutputReduction * 127.0;
    cardinalMax = maxOutputReduction * 127.0;
    saturationRadius = maxOutputReduction * 127.0;
    break;
  }

  //scale {-32767 ... +32767} to {-saturationRadius + offset ... +saturationRadius + offset}
  auto ax = axis->setOperatingRange(aleck64.controls.controllerAxis(playerIndex, "X"), saturationRadius, offset);
  auto ay = axis->setOperatingRange(aleck64.controls.controllerAxis(playerIndex, "Y"), saturationRadius, offset);

  auto configuredNotchLengthFromEdge = axis->notchLengthFromEdge(); //user-defined [0.0, 1.0] (default 0.1); the default value cannot be configured by user in other projects
  auto configuredMaxNotchAngularDist = axis->notchAngularSnappingDistance(); //user-defined in degrees [0.0, 45.0] (default 0.0); values under 15.0 are likely to be more favorable
  auto configuredVirtualNotch = axis->virtualNotch();
  if(configuredVirtualNotch == true) {
    auto initialLength = hypot(ax, ay);
    auto initialAngle = atan2(ay, ax);
    auto currentAngle = axis->virtualNotch(initialLength, initialAngle, saturationRadius, configuredNotchLengthFromEdge, configuredMaxNotchAngularDist);
    ax = cos(currentAngle) * initialLength;
    ay = sin(currentAngle) * initialLength;
  }

  string configuredResponseCurveMode = axis->responseCurve();
  if(configuredResponseCurveMode == "Linear (Default)") response = Response::Linear;
  if(configuredResponseCurveMode == "Relaxed to Aggressive") response = Response::RelaxedToAggressive;
  if(configuredResponseCurveMode == "Aggressive to Relaxed") response = Response::AggressiveToRelaxed;
  if(configuredResponseCurveMode == "Relaxed to Linear") response = Response::RelaxedToLinear;
  if(configuredResponseCurveMode == "Linear to Relaxed") response = Response::LinearToRelaxed;
  if(configuredResponseCurveMode == "Aggressive to Linear") response = Response::AggressiveToLinear;
  if(configuredResponseCurveMode == "Linear to Aggressive") response = Response::LinearToAggressive;

  auto configuredRangeNormalizedInflectionPoint = std::clamp(axis->rangeNormalizedInflectionPoint(), 0.005, 0.995); //user-defined (configuredInnerDeadzone, cardinalMax) (default 50.0)
  auto configuredResponseStrength = axis->responseStrength(); //user-defined (0.0, 100.0] (default 100.0); used to produce a more relaxed or aggressive curve; values close to 0.0 and 100.0 create strongest response when relaxed and aggressive, respectively
  auto configuredProportionalSensitivity = axis->proportionalSensitivity(); //user-defined (default 1.0); Should this only apply to a linear response? What percentage range should be used? Place outside of Gamepad::responseCurve()?

  //create inner axial dead-zone in range {-configuredInnerDeadzone ... +configuredInnerDeadzone} and scale from it up to saturationRadius
  if(configuredDeadzoneShape == "Axial") {
    auto lengthAbsoluteX = abs(ax - offset);
    auto lengthAbsoluteY = abs(ay - offset);
    ax = axis->processDeadzoneAndResponseCurve(ax, lengthAbsoluteX, configuredInnerDeadzone, saturationRadius, offset, configuredRangeNormalizedInflectionPoint, configuredResponseStrength, configuredProportionalSensitivity, configuredResponseCurveMode);
    ay = axis->processDeadzoneAndResponseCurve(ay, lengthAbsoluteY, configuredInnerDeadzone, saturationRadius, offset, configuredRangeNormalizedInflectionPoint, configuredResponseStrength, configuredProportionalSensitivity, configuredResponseCurveMode);
  } else {
    auto length = hypot(ax - offset, ay - offset);
    ax = axis->processDeadzoneAndResponseCurve(ax, length, configuredInnerDeadzone, saturationRadius, offset, configuredRangeNormalizedInflectionPoint, configuredResponseStrength, configuredProportionalSensitivity, configuredResponseCurveMode);
    ay = axis->processDeadzoneAndResponseCurve(ay, length, configuredInnerDeadzone, saturationRadius, offset, configuredRangeNormalizedInflectionPoint, configuredResponseStrength, configuredProportionalSensitivity, configuredResponseCurveMode);
  }

  auto scaledLength = hypot(ax - offset, ay - offset);
  if(scaledLength > saturationRadius) {
    scaledLength = saturationRadius / scaledLength;
    ax = axis->revisePosition(ax, scaledLength, offset);
    ay = axis->revisePosition(ay, scaledLength, offset);
  }

  //let cardinalMax and diagonalMax define boundaries and restrict to an octagonal gate
  if(outputStyle == OutputStyle::VirtualOctagon || outputStyle == OutputStyle::CustomVirtualOctagon || outputStyle == OutputStyle::MorphedOctagon || outputStyle == OutputStyle::CustomMorphedOctagon || outputStyle == OutputStyle::MaxVirtualSquare || outputStyle == OutputStyle::MaxMorphedSquare) {
    if(ax != offset && ay != offset) {
      double edgex = 0.0;
      double edgey = 0.0;

      axis->applyGateBoundaries(cardinalMax, diagonalMax, ax, ay, offset, edgex, edgey);

      auto distanceToEdge = hypot(edgex, edgey);

      if(outputStyle == OutputStyle::VirtualOctagon || outputStyle == OutputStyle::CustomVirtualOctagon || outputStyle == OutputStyle::MaxVirtualSquare) {
        auto currentLength = hypot(ax - offset, ay - offset);
        if(currentLength > distanceToEdge) {
          ax = edgex;
          ay = edgey;
        }
      } else {
        auto scale = distanceToEdge / saturationRadius;
        ax = axis->revisePosition(ax, scale, offset);
        ay = axis->revisePosition(ay, scale, offset);
      }
    }
  }

  //keep cardinal input within positive and negative bounds of cardinalMax
  if(outputStyle == OutputStyle::VirtualOctagon || outputStyle == OutputStyle::CustomVirtualOctagon || outputStyle == OutputStyle::MaxVirtualSquare) {
    ax = axis->clampAxisToNearestBoundary(ax, offset, cardinalMax);
    ay = axis->clampAxisToNearestBoundary(ay, offset, cardinalMax);
  }

  //add epsilon to counteract floating point precision error
  ax = axis->counteractPrecisionError(ax);
  ay = axis->counteractPrecisionError(ay);
  
  n32 data;
  data.byte(0) = s8(-ay);
  data.byte(1) = s8(+ax);
  data.bit(16) = aleck64.controls.controllerButton(playerIndex, "C-Right");
  data.bit(17) = aleck64.controls.controllerButton(playerIndex, "C-Left");
  data.bit(18) = aleck64.controls.controllerButton(playerIndex, "C-Down");
  data.bit(19) = aleck64.controls.controllerButton(playerIndex, "C-Up");
  data.bit(20) = aleck64.controls.controllerButton(playerIndex, "R");
  data.bit(21) = aleck64.controls.controllerButton(playerIndex, "L");
  data.bit(22) = 0;  //GND
  data.bit(23) = 0;  //RST
  data.bit(24) = aleck64.controls.controllerButton(playerIndex, "Right");
  data.bit(25) = aleck64.controls.controllerButton(playerIndex, "Left");
  data.bit(26) = aleck64.controls.controllerButton(playerIndex, "Down");
  data.bit(27) = aleck64.controls.controllerButton(playerIndex, "Up");
  data.bit(28) = aleck64.controls.controllerButton(playerIndex, "Start");
  data.bit(29) = aleck64.controls.controllerButton(playerIndex, "Z");
  data.bit(30) = aleck64.controls.controllerButton(playerIndex, "B");
  data.bit(31) = aleck64.controls.controllerButton(playerIndex, "A");

  return data;
}
