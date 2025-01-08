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

  auto cardinalMax   = 85.0;
  auto diagonalMax   = 69.0;
  auto innerDeadzone =  7.0; // default should remain 7 (~8.2% of 85) as the deadzone is axial in nature and fights cardinalMax
  auto outerDeadzoneRadiusMax = 2.0 / sqrt(2.0) * (diagonalMax / cardinalMax * (cardinalMax - innerDeadzone) + innerDeadzone); //from linear scaling equation, substitute outerDeadzoneRadiusMax*sqrt(2)/2 for lengthAbsoluteX and set diagonalMax as the result then solve for outerDeadzoneRadiusMax

  //scale {-32768 ... +32767} to {-outerDeadzoneRadiusMax ... +outerDeadzoneRadiusMax}
  auto ax = aleck64.controls.controllerAxis(playerIndex, "X") * outerDeadzoneRadiusMax / 32767.0;
  auto ay = aleck64.controls.controllerAxis(playerIndex, "Y") * outerDeadzoneRadiusMax / 32767.0;
  
  //create inner axial dead-zone in range {-innerDeadzone ... +innerDeadzone} and scale from it up to outer circular dead-zone of radius outerDeadzoneRadiusMax
  auto length = sqrt(ax * ax + ay * ay);
  if(length <= outerDeadzoneRadiusMax) {
    auto lengthAbsoluteX = abs(ax);
    auto lengthAbsoluteY = abs(ay);
    if(lengthAbsoluteX <= innerDeadzone) {
      lengthAbsoluteX = 0.0;
    } else {
      lengthAbsoluteX = (lengthAbsoluteX - innerDeadzone) * cardinalMax / (cardinalMax - innerDeadzone) / lengthAbsoluteX;
    }
    ax *= lengthAbsoluteX;
    if(lengthAbsoluteY <= innerDeadzone) {
      lengthAbsoluteY = 0.0;
    } else {
      lengthAbsoluteY = (lengthAbsoluteY - innerDeadzone) * cardinalMax / (cardinalMax - innerDeadzone) / lengthAbsoluteY;
    }
    ay *= lengthAbsoluteY;
  } else {
    length = outerDeadzoneRadiusMax / length;
    ax *= length;
    ay *= length;
  }
  
  //bound diagonals to an octagonal range {-diagonalMax ... +diagonalMax}
  if(ax != 0.0 && ay != 0.0) {
    auto slope = ay / ax;
    auto edgex = copysign(cardinalMax / (abs(slope) + (cardinalMax - diagonalMax) / diagonalMax), ax);
    auto edgey = copysign(min(abs(edgex * slope), cardinalMax / (1.0 / abs(slope) + (cardinalMax - diagonalMax) / diagonalMax)), ay);
    edgex = edgey / slope;

    length = sqrt(ax * ax + ay * ay);
    auto distanceToEdge = sqrt(edgex * edgex + edgey * edgey);
    if(length > distanceToEdge) {
      ax = edgex;
      ay = edgey;
    }
  }

  //keep cardinal input within positive and negative bounds of cardinalMax
  if(abs(ax) > cardinalMax) ax = copysign(cardinalMax, ax);
  if(abs(ay) > cardinalMax) ay = copysign(cardinalMax, ay);
  
  //add epsilon to counteract floating point precision error
  ax = copysign(abs(ax) + 1e-09, ax);
  ay = copysign(abs(ay) + 1e-09, ay);
  
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
