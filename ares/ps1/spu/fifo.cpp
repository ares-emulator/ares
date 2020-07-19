auto SPU::fifoManualWrite() -> void {
  if(transfer.type == 2) {
    while(fifo.size() >= 1) {
      u16 data = fifo.read();
      writeRAM(transfer.current, data);
      transfer.current += 2;
    }
  }

  if(transfer.type == 3) {
    while(fifo.size() >= 2) {
      u16 data = fifo.read();
      fifo.read();
      writeRAM(transfer.current + 0, data);
      writeRAM(transfer.current + 2, data);
      transfer.current += 4;
    }
  }

  if(transfer.type == 4) {
    while(fifo.size() >= 4) {
      u16 data = fifo.read();
      fifo.read();
      fifo.read();
      fifo.read();
      writeRAM(transfer.current + 0, data);
      writeRAM(transfer.current + 2, data);
      writeRAM(transfer.current + 4, data);
      writeRAM(transfer.current + 6, data);
      transfer.current += 8;
    }
  }

  if(transfer.type == 5) {
    while(fifo.size() >= 8) {
      fifo.read();
      fifo.read();
      fifo.read();
      fifo.read();
      fifo.read();
      fifo.read();
      fifo.read();
      u16 data = fifo.read();
      writeRAM(transfer.current +  0, data);
      writeRAM(transfer.current +  2, data);
      writeRAM(transfer.current +  4, data);
      writeRAM(transfer.current +  6, data);
      writeRAM(transfer.current +  8, data);
      writeRAM(transfer.current + 10, data);
      writeRAM(transfer.current + 12, data);
      writeRAM(transfer.current + 14, data);
      transfer.current += 16;
    }
  }

  if(transfer.type <= 1 || transfer.type >= 6) {
    if(u16 size = fifo.size()) {
      u16 data = 0;
      while(!fifo.empty()) data = fifo.read();
      while(size--) {
        writeRAM(transfer.current, data);
        transfer.current += 2;
      }
    }
  }
}
