auto Display::readIO(n32 address) -> n8 {
  switch(address) {

  //DISPSTAT
  case 0x0400'0004: return (
    io.vblank          << 0
  | io.hblank          << 1
  | io.vcoincidence    << 2
  | io.irqvblank       << 3
  | io.irqhblank       << 4
  | io.irqvcoincidence << 5
  );
  case 0x0400'0005: return (
    io.vcompare
  );

  //VCOUNT
  case 0x0400'0006: return io.vcounter.byte(0);
  case 0x0400'0007: return io.vcounter.byte(1);

  }

  return cpu.openBus.get(Byte, address);
}

auto Display::writeIO(n32 address, n8 data) -> void {
  switch(address) {

  //DISPSTAT
  case 0x0400'0004:
    io.irqvblank       = data.bit(3);
    io.irqhblank       = data.bit(4);
    io.irqvcoincidence = data.bit(5);
    return;
  case 0x0400'0005:
    io.vcompare = data;
    return;

  }
}
