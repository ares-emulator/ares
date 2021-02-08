auto SPC7110::aluMultiply() -> void {
  addClocks(30);

  if(r482e & 1) {
    //signed 16-bit x 16-bit multiplication
    i16 r0 = (i16)(r4824 | r4825 << 8);
    i16 r1 = (i16)(r4820 | r4821 << 8);

    i32 result = r0 * r1;
    r4828 = result;
    r4829 = result >> 8;
    r482a = result >> 16;
    r482b = result >> 24;
  } else {
    //unsigned 16-bit x 16-bit multiplication
    n16 r0 = (n16)(r4824 | r4825 << 8);
    n16 r1 = (n16)(r4820 | r4821 << 8);

    n32 result = r0 * r1;
    r4828 = result;
    r4829 = result >> 8;
    r482a = result >> 16;
    r482b = result >> 24;
  }

  r482f &= 0x7f;
}

auto SPC7110::aluDivide() -> void {
  addClocks(40);

  if(r482e & 1) {
    //signed 32-bit x 16-bit division
    i32 dividend = (i32)(r4820 | r4821 << 8 | r4822 << 16 | r4823 << 24);
    i16 divisor  = (i16)(r4826 | r4827 << 8);

    i32 quotient;
    i16 remainder;

    if(divisor) {
      quotient  = (i32)(dividend / divisor);
      remainder = (i32)(dividend % divisor);
    } else {
      //illegal division by zero
      quotient  = 0;
      remainder = dividend;
    }

    r4828 = quotient;
    r4829 = quotient >> 8;
    r482a = quotient >> 16;
    r482b = quotient >> 24;

    r482c = remainder;
    r482d = remainder >> 8;
  } else {
    //unsigned 32-bit x 16-bit division
    n32 dividend = (n32)(r4820 | r4821 << 8 | r4822 << 16 | r4823 << 24);
    n16 divisor  = (n16)(r4826 | r4827 << 8);

    n32 quotient;
    n16 remainder;

    if(divisor) {
      quotient  = (n32)(dividend / divisor);
      remainder = (n16)(dividend % divisor);
    } else {
      //illegal division by zero
      quotient  = 0;
      remainder = dividend;
    }

    r4828 = quotient;
    r4829 = quotient >> 8;
    r482a = quotient >> 16;
    r482b = quotient >> 24;

    r482c = remainder;
    r482d = remainder >> 8;
  }

  r482f &= 0x7f;
}
