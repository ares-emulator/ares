auto VDP::FIFO::slot() -> void {
  if(slots.size() < 7) vdp.dma.run();
  if(slots.size() < 2) return;

  auto slot = *slots.read();

  if(slot.target == 1) {
    vdp.vram.writeByte(slot.address, slot.data);
  }

  if(slot.target == 3) {
    n16 data;
    data.byte(1) = slot.data;
    slot = *slots.read();
    data.byte(0) = slot.data;
    vdp.cram.write(slot.address >> 1, data.bit(1,3) << 0 | data.bit(5,7) << 3 | data.bit(9,11) << 6);
  }

  if(slot.target == 5) {
    n16 data;
    data.byte(1) = slot.data;
    slot = *slots.read();
    data.byte(0) = slot.data;
    vdp.vsram.write(slot.address >> 1, data);
  }
}

auto VDP::FIFO::write(n17 address, n8 data, n4 target) -> void {
  if(slots.full()) {
    bus.acquire(Bus::VDPFIFO);
    while(slots.full()) cpu.wait(1);
    bus.release(Bus::VDPFIFO);
  }
  slots.write({address, data, target});
}
