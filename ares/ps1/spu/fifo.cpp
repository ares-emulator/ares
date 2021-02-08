auto SPU::fifoReadBlock() -> void {
  if(transfer.type == 2) {
    for(u32 from : range(8)) {
      u16 data = readRAM(transfer.current);
      fifo.write(data);
      transfer.current += 2;
    }
  }

  if(transfer.type == 3) {
    for(u32 from : range(4)) {
      u16 data = readRAM(transfer.current);
      for(u32 to : range(2)) fifo.write(data);
      transfer.current += 4;
    }
  }

  if(transfer.type == 4) {
    for(u32 from : range(2)) {
      u16 data = readRAM(transfer.current);
      for(u32 to : range(4)) fifo.write(data);
      transfer.current += 8;
    }
  }

  if(transfer.type == 5) {
    u16 data = readRAM(transfer.current);
    for(u32 to : range(8)) fifo.write(data);
    transfer.current += 16;
  }

  if(transfer.type <= 1 || transfer.type >= 6) {
    u16 data = readRAM(transfer.current + 14);
    for(u32 to : range(8)) fifo.write(data);
    transfer.current += 16;
  }
}

auto SPU::fifoWriteBlock() -> void {
  u16 data[8];
  for(auto& half : data) half = fifo.read(0);

  if(transfer.type == 2) {
    for(u32 from : range(8)) {
      writeRAM(transfer.current, data[from]);
      transfer.current += 2;
    }
  }

  if(transfer.type == 3) {
    for(u32 from : range(4)) {
      for(u32 to : range(2)) {
        writeRAM(transfer.current, data[from * 2]);
        transfer.current += 2;
      }
    }
  }

  if(transfer.type == 4) {
    for(u32 from : range(2)) {
      for(u32 to : range(4)) {
        writeRAM(transfer.current, data[from * 4]);
        transfer.current += 2;
      }
    }
  }

  if(transfer.type == 5) {
    for(u32 to : range(8)) {
      writeRAM(transfer.current, data[0]);
      transfer.current += 2;
    }
  }

  if(transfer.type <= 1 || transfer.type >= 6) {
    for(u32 to : range(8)) {
      writeRAM(transfer.current, data[7]);
      transfer.current += 2;
    }
  }
}
