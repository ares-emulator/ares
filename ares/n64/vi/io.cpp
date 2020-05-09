static const vector<string> registerNames = {
  "VI_CONTROL",
  "VI_DRAM_ADDR",
  "VI_H_WIDTH",
  "VI_V_INTR",
  "VI_V_CURRENT_LINE",
  "VI_TIMING",
  "VI_V_SYNC",
  "VI_H_SYNC",
  "VI_H_SYNC_LEAP",
  "VI_H_VIDEO",
  "VI_V_VIDEO",
  "VI_V_BURST",
  "VI_X_SCALE",
  "VI_Y_SCALE",
};

auto VI::readIO(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  u32 data = 0;

  if(address == 0) {
    //VI_CONTROL
    data |= io.colorDepth << 0;
  }

  if(address == 1) {
    //VI_DRAM_ADDR
    data |= io.dramAddress << 0;
  }

  if(address == 2) {
    //VI_H_WIDTH
    data |= io.width << 0;
  }

  if(address == 3) {
    //VI_V_INTR
  }

  if(address == 4) {
    //VI_V_CURRENT_LINE
    data |= rdp.io.field    << 0;
    data |= rdp.io.vcounter << 1;
  }

  if(address == 5) {
    //VI_TIMING
  }

  if(address == 6) {
    //VI_V_SYNC
  }

  if(address == 7) {
    //VI_H_SYNC
  }

  if(address == 8) {
    //VI_H_SYNC_LEAP
  }

  if(address == 9) {
    //VI_H_VIDEO
  }

  if(address == 10) {
    //VI_V_VIDEO
  }

  if(address == 11) {
    //VI_V_BURST
  }

  if(address == 12) {
    //VI_X_SCALE
    data |= io.xscale    <<  0;
    data |= io.xsubpixel << 16;
  }

  if(address == 13) {
    //VI_Y_SCALE
    data |= io.yscale    <<  0;
    data |= io.ysubpixel << 16;
  }

//print("* ", registerNames(address, "VI_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto VI::writeIO(u32 address, u32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //VI_CONTROL
    io.colorDepth = uint2(data >> 0);
  }

  if(address == 1) {
    //VI_DRAM_ADDR
    io.dramAddress = uint32(data >> 0);
  }

  if(address == 2) {
    //VI_H_WIDTH
    io.width = uint12(data >> 0);
  }

  if(address == 3) {
    //VI_V_INTR
  }

  if(address == 4) {
    //VI_V_CURRENT_LINE
    mi.irq.vi.line = 0;
    mi.pollInterrupts();
  }

  if(address == 5) {
    //VI_TIMING
  }

  if(address == 6) {
    //VI_V_SYNC
  }

  if(address == 7) {
    //VI_H_SYNC
  }

  if(address == 8) {
    //VI_H_SYNC_LEAP
  }

  if(address == 9) {
    //VI_H_VIDEO
  }

  if(address == 10) {
    //VI_V_VIDEO
  }

  if(address == 11) {
    //VI_V_BURST
  }

  if(address == 12) {
    //VI_X_SCALE
    io.xscale    = uint12(data >>  0);
    io.xsubpixel = uint12(data >> 16);
  }

  if(address == 13) {
    //VI_Y_SCALE
    io.yscale    = uint12(data >>  0);
    io.ysubpixel = uint12(data >> 16);
  }

//print("* ", registerNames(address, "VI_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
