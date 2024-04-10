auto APU::Length::main() -> void {
  if (halt == 0 && counter)
    counter--;

  if (delayHalt) {
    delayHalt = false;
    halt = newHalt;
  }

  if (delayCounter) {
    delayCounter = false;
    if (counter == 0)
      counter = table[counterIndex];
  }
}

auto APU::Length::power(bool reset, bool isTriangle) -> void {
  enable = 0;

  if (!reset || !isTriangle) {
    counter = 0;
    halt = 0;

    delayHalt = false;
    newHalt = 0;

    delayCounter = false;
    counterIndex = 0;
  }
}

auto APU::Length::setEnable(n1 value) -> void {
  enable = value;
  if (!enable)
    counter = 0;
}

auto APU::Length::setHalt(bool lengthClocking, n1 value) -> void {
  if (!lengthClocking) {
    halt = value;
  } else {
    delayHalt = true;
    newHalt = value;
  }
}

auto APU::Length::setCounter(bool lengthClocking, n5 index) -> void {
  if (enable) {
    if (!lengthClocking)
      counter = table[index];
    else {
      delayCounter = true;
      counterIndex = index;
    }
  }
}
