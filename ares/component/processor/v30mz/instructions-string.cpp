template<u32 size> auto V30MZ::instructionInString() -> void {
  wait(5);
  if(!repeat() || CW) {
    auto data = in<size>(DW);
    write<size>(DS1, IY, data);
    IY += PSW.DIR ? -size : size;

    if(!repeat() || !--CW) return;

    state.prefix = 1;
    PC--;
    loop();
  }
}

template<u32 size> auto V30MZ::instructionOutString() -> void {
  wait(6);
  if(!repeat() || CW) {
    auto data = read<size>(segment(DS0), IX);
    out<size>(DW, data);
    IX += PSW.DIR ? -size : size;

    if(!repeat() || !--CW) return;

    state.prefix = 1;
    PC--;
    loop();
  }
}

template<u32 size> auto V30MZ::instructionMoveString() -> void {
  wait(3);
  if(!repeat() || CW) {
    auto data = read<size>(segment(DS0), IX);
    write<size>(DS1, IY, data);
    IX += PSW.DIR ? -size : size;
    IY += PSW.DIR ? -size : size;

    if(!repeat() || !--CW) return;

    state.prefix = 1;
    PC--;
    loop();
  }
}

template<u32 size> auto V30MZ::instructionCompareString() -> void {
  wait(4);
  if(!repeat() || CW) {
    auto x = read<size>(segment(DS0), IX);
    auto y = read<size>(DS1, IY);
    IX += PSW.DIR ? -size : size;
    IY += PSW.DIR ? -size : size;
    SUB<size>(x, y);

    if(!repeat() || !--CW) return;
    if(repeat() == RepeatWhileZeroLo && PSW.Z == 1) return;
    if(repeat() == RepeatWhileZeroHi && PSW.Z == 0) return;

    state.prefix = 1;
    PC--;
    loop();
  }
}

template<u32 size> auto V30MZ::instructionStoreString() -> void {
  wait(2);
  if(!repeat() || CW) {
    write<size>(DS1, IY, getAccumulator<size>());
    IY += PSW.DIR ? -size : size;

    if(!repeat() || !--CW) return;

    state.prefix = 1;
    PC--;
    loop();
  }
}

template<u32 size> auto V30MZ::instructionLoadString() -> void {
  wait(2);
  if(!repeat() || CW) {
    setAccumulator<size>(read<size>(segment(DS0), IX));
    IX += PSW.DIR ? -size : size;

    if(!repeat() || !--CW) return;

    state.prefix = 1;
    PC--;
    loop();
  }
}

template<u32 size> auto V30MZ::instructionScanString() -> void {
  wait(3);
  if(!repeat() || CW) {
    auto x = getAccumulator<size>();
    auto y = read<size>(DS1, IY);
    IY += PSW.DIR ? -size : size;
    SUB<size>(x, y);

    if(!repeat() || !--CW) return;
    if(repeat() == RepeatWhileZeroLo && PSW.Z == 1) return;
    if(repeat() == RepeatWhileZeroHi && PSW.Z == 0) return;

    state.prefix = 1;
    PC--;
    loop();
  }
}
