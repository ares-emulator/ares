auto MCD::CDC::Transfer::dma() -> void {
  if(!active) return;
  if(destination != 4 && destination != 5 && destination != 7) return;

  uint16 data = mcd.cdc.ram[uint13(source >> 1)];
  switch(destination) {
  case 4:  //PCM (0x1000 - 0x1fff = PCM RAM active 4KB bank)
    mcd.pcm.write(0x1000 | uint12(address >> 1) | 1, data.byte(1));
    mcd.pcm.write(0x1000 | uint12(address >> 1) | 0, data.byte(0));
    address += 2;  //PCM DMA requires two 8-bit writes per transfer
    break;

  case 5:  //PRAM
    mcd.write(1, 1, 0x000000 | (uint19)address & ~1, data);
    break;

  case 7:  //WRAM
    mcd.write(1, 1, 0x080000 | (uint18)address & ~1, data);
    break;
  }

  address += 2;
  source  += 2;
  if(!length--) complete();
  if(!length--) complete();
}

auto MCD::CDC::Transfer::read() -> uint16 {
  if(!ready) return 0xffff;

  uint16 data = mcd.cdc.ram[uint13(source >> 1)];
  source += 2;
  if(!length--) complete();
  if(!length--) complete();
  return data;
}

auto MCD::CDC::Transfer::start() -> void {
  if(!enable) return;

  active = 1;
  busy = 1;
  ready = (destination == 2 || destination == 3);
  completed = 0;
  mcd.cdc.irq.transfer.pending = 0;
  mcd.cdc.poll();
}

auto MCD::CDC::Transfer::complete() -> void {
  active = 0;
  busy = 0;
  ready = 0;
  completed = 1;
  mcd.cdc.irq.transfer.pending = 1;
  mcd.cdc.poll();
}

auto MCD::CDC::Transfer::stop() -> void {
  active = 0;
  busy = 0;
  ready = 0;
}
