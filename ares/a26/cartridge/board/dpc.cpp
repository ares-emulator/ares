struct DPC : Interface, Thread {
  using Interface::Interface;

  struct Fetcher {
    n8 top;
    n8 bottom;
    n8 low;
    n3 high;
    n1 flag;
    n1 musicMode;
    n1 oscillatorClock;
  } fetcher[8];

  Memory::Readable<n8> rom;
  n1 bank;
  n8 random;
  n4 movement;
  n8 drawLatch;
  n8 drawOperand;
  n1 drawCarry;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto clockRandom() -> void {
    n1 input = !((random.bit(7) ^ random.bit(5)) ^ (random.bit(4) ^ random.bit(3)));
    random = random << 1 | input;
  }

  auto checkFlag(u32 index) -> void {
    if(fetcher[index].low == fetcher[index].top) fetcher[index].flag = 1;
    if(fetcher[index].low == fetcher[index].bottom) fetcher[index].flag = 0;
  }

  auto decrement(u32 index) -> void {
    if(--fetcher[index].low == 0xff) {
      fetcher[index].high--;
      if(index >= 5 && fetcher[index].musicMode) fetcher[index].low = fetcher[index].top;
    }
    checkFlag(index);
  }

  auto display(u32 index) -> n8 {
    auto counter = fetcher[index].high << 8 | fetcher[index].low;
    return rom.read(8_KiB + (0x7ff - counter));
  }

  auto music() -> n4 {
    static constexpr u8 amplitude[] = {0x0, 0x4, 0x5, 0x9, 0x6, 0xa, 0xb, 0xf};
    u32 state = 0;
    if(fetcher[5].musicMode && fetcher[5].flag) state |= 1;
    if(fetcher[6].musicMode && fetcher[6].flag) state |= 2;
    if(fetcher[7].musicMode && fetcher[7].flag) state |= 4;
    return amplitude[state];
  }

  auto reverse(n8 data) -> n8 {
    n8 result;
    for(u32 bit : range(8)) result.bit(7 - bit) = data.bit(bit);
    return result;
  }

  auto readRegister(n16 address) -> n8 {
    auto index = address & 7;
    auto function = address.bit(3, 5);

    if(function == 0) {
      if(index < 4) return random;
      if(index <= 5) {
        drawOperand = drawLatch;
        auto sum = u32{drawOperand} + u32{fetcher[4].top};
        drawLatch = sum;
        drawCarry = sum > 0xff;
      }
      return n8{drawCarry ? movement << 4 : 0} | music();
    }

    checkFlag(index);
    auto data = display(index);
    n8 result;
    if(function == 1) result = data;
    if(function == 2) result = fetcher[index].flag ? data : n8{0};
    if(function == 3) result = fetcher[index].flag ? n8{data << 4 | data >> 4} : n8{0};
    if(function == 4) result = fetcher[index].flag ? reverse(data) : n8{0};
    if(function == 5) result = fetcher[index].flag ? n8{data >> 1} : n8{0};
    if(function == 6) result = fetcher[index].flag ? n8{data << 1} : n8{0};
    if(function == 7) result = fetcher[index].flag ? 0xff : 0x00;

    if(index < 5 || !fetcher[index].oscillatorClock) decrement(index);
    return result;
  }

  auto writeRegister(n16 address, n8 data) -> void {
    auto index = address & 7;
    auto function = address.bit(3, 5);

    if(function == 0) {
      fetcher[index].top = data;
      fetcher[index].flag = 0;
    }
    if(function == 1) {
      fetcher[index].bottom = data;
      checkFlag(index);
    }
    if(function == 2) {
      fetcher[index].low = index >= 5 && fetcher[index].musicMode ? fetcher[index].top : data;
      if(index == 4) drawLatch = data;
      checkFlag(index);
    }
    if(function == 3) {
      fetcher[index].high = data.bit(0, 2);
      if(index >= 5) {
        fetcher[index].musicMode = data.bit(4);
        fetcher[index].oscillatorClock = data.bit(5);
        if(fetcher[index].musicMode && fetcher[index].low == 0xff) {
          fetcher[index].low = fetcher[index].top;
          checkFlag(index);
        }
      }
    }
    if(function == 4) movement = data.bit(4, 7);
    if(function == 6) random = 0;
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) return data;
    clockRandom();

    if(address == 0x1ff8) bank = 0;
    if(address == 0x1ff9) bank = 1;
    if(address <= 0x103f) return readRegister(address);
    return rom.read(bank * 4_KiB + (address & 0xfff));
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) return data;
    clockRandom();

    if(address == 0x1ff8) bank = 0;
    if(address == 0x1ff9) bank = 1;
    if(address >= 0x1040 && address <= 0x107f) {
      writeRegister(address, data);
      return data;
    }
    return data;
  }

  auto main() -> void {
    for(u32 index = 5; index < 8; index++) {
      if(fetcher[index].oscillatorClock) decrement(index);
    }
    step(1);
    Thread::synchronize(cpu);
  }

  auto power(bool reset) -> void override {
    Thread::create(20'000, std::bind_front(&DPC::main, this));
    //Stella latch bank 1.
    //MAME latch bank 0.
    bank = 1;
    random = 0;
    movement = 0;
    drawLatch = 0;
    drawOperand = 0;
    drawCarry = 0;
    for(auto& item : fetcher) item = {};
  }

  auto serialize(serializer& s) -> void override {
    Thread::serialize(s);
    s(bank);
    s(random);
    s(movement);
    s(drawLatch);
    s(drawOperand);
    s(drawCarry);
    for(auto& item : fetcher) {
      s(item.top);
      s(item.bottom);
      s(item.low);
      s(item.high);
      s(item.flag);
      s(item.musicMode);
      s(item.oscillatorClock);
    }
  }
};
