auto RIOT::readRam(n8 address) -> n8 {
  return ram[address];
}

auto RIOT::writeRam(n8 address, n8 data) -> void {
  ram[address] = data;
}

auto RIOT::readIo(n8 address) -> n8 {
  address &= 0x1f;
  n8 portA = readPortA();
  samplePA7(portA.bit(7));

  if(!address.bit(2)) {
    switch(address.bit(0, 1)) {
    case 0: return portA;
    case 1: return port[0].direction;
    case 2: return readPortB();
    case 3: return port[1].direction;
    }
  }

  if(!address.bit(0)) {
    timer.interruptEnable = address.bit(3);
    n8 data = timer.counter;

    if(timer.interruptFlag && !timer.justWrapped) {
      timer.interruptFlag = 0;
      timer.prescaler = 1;
    }

    return data;
  }

  n8 data = 0x00;
  data.bit(6) = pa7.interruptFlag;
  data.bit(7) = timer.interruptFlag;
  pa7.interruptFlag = 0;
  return data;
}

auto RIOT::writeIo(n8 address, n8 data) -> void {
  address &= 0x1f;
  samplePA7(readPortA().bit(7));

  if(!address.bit(2)) {
    switch(address.bit(0, 1)) {
    case 0: writePortA(data);         return;
    case 1: writeDirectionA(data);    return;
    case 2: writePortB(data);         return;
    case 3: port[1].direction = data; return;
    }
  }

  if(!address.bit(4)) {
    pa7.positiveEdge = address.bit(0);
    pa7.interruptEnable = address.bit(1);
    return;
  }

  static constexpr u16 interval[4] = {1, 8, 64, 1024};
  reloadTimer(data, interval[address.bit(0, 1)], address.bit(3));
}

auto RIOT::readPortA() -> n8 {
  n8 input;
  input.bit(0, 3) = controllerPort2.read().bit(0, 4);
  input.bit(4, 7) = controllerPort1.read().bit(0, 4);

  return (port[0].data | ~port[0].direction) & input;
}

auto RIOT::writePortA(n8 data) -> void {
  port[0].data = data;
  drivePortA();
  samplePA7(readPortA().bit(7));
}

auto RIOT::writeDirectionA(n8 data) -> void {
  port[0].direction = data;
  drivePortA();
  samplePA7(readPortA().bit(7));
}

auto RIOT::drivePortA() -> void {
  n8 output = port[0].data | ~port[0].direction;
  controllerPort1.write(output.bit(4, 7));
  controllerPort2.write(output.bit(0, 3));
  for(auto index : range(4)) tia.updateAnalogInput(index);
  for(auto index : range(2)) tia.updateTriggerInput(index);
}

auto RIOT::samplePA7(n1 level) -> void {
  if(level != pa7.level && level == pa7.positiveEdge) pa7.interruptFlag = 1;
  pa7.level = level;
}

auto RIOT::readPortB() -> n8 {
  system.controls.poll();

  n8 input = 0xff;

  input.bit(0) = !system.controls.reset->value();
  input.bit(1) = !system.controls.select->value();

  //These are toggle switches; flip the values on the rising edge
  if(system.controls.tvType->value() && !tvTypeLatch) {
    tvType = !tvType;
  }
  tvTypeLatch = system.controls.tvType->value();

  if(system.controls.leftDifficulty->value() && !leftDifficultyLatch) {
    leftDifficulty = !leftDifficulty;
  }
  leftDifficultyLatch = system.controls.leftDifficulty->value();

  if(system.controls.rightDifficulty->value() && !rightDifficultyLatch) {
    rightDifficulty = !rightDifficulty;
  }
  rightDifficultyLatch = system.controls.rightDifficulty->value();

  input.bit(3) = tvType;
  input.bit(6) = leftDifficulty;
  input.bit(7) = rightDifficulty;

  return (port[1].data & port[1].direction) | (input & ~port[1].direction);
}

auto RIOT::writePortB(n8 data) -> void {
  port[1].data = data;
}
