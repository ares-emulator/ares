auto VDP::FIFO::slot() -> void {
  if(requests.size() >= 2) {
    auto slot = *requests.read();

    if(slot.target == 0) {
      response = response << 8 | vdp.vram.readByte(slot.address);
      return;
    }

    if(slot.target == 4) {
      response = vdp.vsram.read(slot.address >> 1);
      slot = *requests.read();
      return;
    }

    if(slot.target == 8) {
      response = vdp.cram.read(slot.address >> 1);
      response = response.bit(0,2) << 1 | response.bit(3,5) << 5 | response.bit(6,8) << 9;
      slot = *requests.read();
      return;
    }

    debug(unusual, "[VDP::FIFO] slot.target = 0x", hex(slot.target));
    return;
  }

  if(slots.size() < 7) {
    vdp.dma.run();
  }

  if(slots.size() >= 2) {
    auto slot = *slots.read();

    if(slot.target == 1) {
      vdp.vram.writeByte(slot.address, slot.data);
      return;
    }

    if(slot.target == 3) {
      n16 data;
      data.byte(1) = slot.data;
      slot = *slots.read();
      data.byte(0) = slot.data;
      data = data.bit(1,3) << 0 | data.bit(5,7) << 3 | data.bit(9,11) << 6;
      vdp.cram.write(slot.address >> 1, data);
      return;
    }

    if(slot.target == 5) {
      n16 data;
      data.byte(1) = slot.data;
      slot = *slots.read();
      data.byte(0) = slot.data;
      vdp.vsram.write(slot.address >> 1, data);
      return;
    }

    debug(unusual, "[VDP::FIFO] slot.target = 0x", hex(slot.target));
    return;
  }
}

auto VDP::FIFO::read(n4 target, n17 address) -> void {
  if(requests.full()) {
    bus.acquire(Bus::VDPFIFO);
    while(requests.full()) cpu.wait(1);
    bus.release(Bus::VDPFIFO);
  }
  requests.write({target, address});
}

auto VDP::FIFO::write(n4 target, n17 address, n8 data) -> void {
  if(slots.full()) {
    bus.acquire(Bus::VDPFIFO);
    while(slots.full()) cpu.wait(1);
    bus.release(Bus::VDPFIFO);
  }
  slots.write({target, address, data});
}

auto VDP::FIFO::power(bool reset) -> void {
  slots.flush();
  requests.flush();
  response = 0;
}
