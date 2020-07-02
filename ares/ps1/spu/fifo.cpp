auto SPU::fifoReset() -> void {
  fifo.size = 0;
}

auto SPU::fifoWrite(u16 data) -> void {
  fifo.data[fifo.size++ & 31] = data;
}

auto SPU::fifoManualWrite() -> void {
  for(uint index : range(min(32, fifo.size))) {
    u16 data = 0;
    switch(transfer.type) {
    case 0: break;  //unknown
    case 1: break;  //unknown
    case 2: data = fifo.data[index]; break;
    case 3: data = fifo.data[index & ~1]; break;
    case 4: data = fifo.data[index & ~3]; break;
    case 5: data = fifo.data[index & ~7 | 7]; break;
    case 6: break;  //unknown
    case 7: break;  //unknown
    }
    ram.writeHalf(transfer.address, data);
    transfer.address += 2;
  }
  fifoReset();
}
