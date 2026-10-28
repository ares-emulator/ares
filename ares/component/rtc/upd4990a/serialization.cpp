auto uPD4990A::serialize(serializer& s) -> void {
  s(data);
  s(clockPin);
  s(strobePin);
  s(shiftRegister);
  s(commandRegister);
  s(command);
  s(outputBit);
  s(timePulseOutput);
  s(counter);
  s(timePulseMode);
  s(timePulseCounter);
}
