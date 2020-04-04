Competition competition;
#include "serialization.cpp"

auto Competition::main() -> void {
  if(scoreActive && scoreSecondsRemaining) {
    if(--scoreSecondsRemaining == 0) {
      scoreActive = false;
    }
  }

  if(timerActive && timerSecondsRemaining) {
    if(--timerSecondsRemaining == 0) {
      timerActive = false;
      status |= 0x02;  //time over
      scoreActive = true;
      scoreSecondsRemaining = 5;
    }
  }

  Thread::step(1);
  Thread::synchronize(cpu);
}

auto Competition::unload() -> void {
  rom[0].reset();
  rom[1].reset();
  rom[2].reset();
  rom[3].reset();
  cpu.coprocessors.removeByValue(this);
  Thread::destroy();
}

auto Competition::power() -> void {
  Thread::create(1, {&Competition::main, this});
  cpu.coprocessors.append(this);

  //DIP switches 0-3 control the time: 3 minutes + 0-15 extra minutes
  timer = (3 + dip.value.bit(0,3)) * 60;  //in seconds
  //DIP switches 4-5 serve an unknown purpose
  //DIP switches 6-7 are not connected

  status = 0x00;
  select = 0x00;
  timerActive = false;
  scoreActive = false;
  timerSecondsRemaining = 0;
  scoreSecondsRemaining = 0;
}

auto Competition::mcuRead(uint24 address, uint8 data) -> uint8 {
  if(board == Board::CampusChallenge92) {
    uint id = 0;
    if(select == 0x09) id = 1;
    if(select == 0x05) id = 2;
    if(select == 0x03) id = 3;
    if((address & 0x808000) == 0x808000) id = 0;

    if(address & 0x008000) {
      address = ((address & 0x7f0000) >> 1) | (address & 0x7fff);
      return rom[id].read(bus.mirror(address, rom[id].size()), data);
    }
  }

  if(board == Board::PowerFest94) {
    uint id = 0;
    if(select == 0x09) id = 1;
    if(select == 0x0c) id = 2;
    if(select == 0x0a) id = 3;
    if((address & 0x208000) == 0x208000) id = 0;

    if(address & 0x400000) {
      address &= 0x3fffff;
      return rom[id].read(bus.mirror(address, rom[id].size()), data);
    }

    if(address & 0x008000) {
      address &= 0x1fffff;
      if(id != 2) address = ((address & 0x1f0000) >> 1) | (address & 0x7fff);
      return rom[id].read(bus.mirror(address, rom[id].size()), data);
    }
  }

  return data;
}

auto Competition::mcuWrite(uint24 address, uint8 data) -> void {
}

auto Competition::read(uint24 address, uint8 data) -> uint8 {
  if(address == 0x106000 || address == 0xc00000) {
    return status;
  }
  return data;
}

auto Competition::write(uint24 address, uint8 data) -> void {
  if(address == 0x206000 || address == 0xe00000) {
    select = data;
    if(timer && data == 0x09) {
      timerActive = true;
      timerSecondsRemaining = timer;
    }
  }
}
