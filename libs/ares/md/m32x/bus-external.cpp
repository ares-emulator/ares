auto M32X::readExternal(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  if(!Mega32X() || !io.adapterEnable) return data;

  if(address >= 0x000000 && address <= 0x0000ff) {
    if(address == 0x70) return io.vectorLevel4 >> 16;
    if(address == 0x72) return io.vectorLevel4 >>  0;
    return vectors[address >> 1];
  }

  if(address >= 0x840000 && address <= 0x87ffff) {
    if(vdp.framebufferAccess) return data;
    if(vdp.framebufferEngaged()) { debug(unusual, "[32X FB] 68k read while FEN==1"); return data; } // wait instead?
    return vdp.bbram[address >> 1 & 0xffff];
  }

  if(address >= 0x880000 && address <= 0x8fffff) {
    if(!dreq.vram) {
      address &= 0x7ffff;
      return cartridge.child->read(upper, lower, address, data);
    }
  }

  if(address >= 0x900000 && address <= 0x9fffff) {
    if(!dreq.vram) {
      address = io.romBank * 0x100000 | address & 0x0fffff;
      return cartridge.child->read(upper, lower, address, data);
    }
  }

  return data;
}

auto M32X::writeExternal(n1 upper, n1 lower, n24 address, n16 data) -> void {
  if(!Mega32X() || !io.adapterEnable) return;

  if(address >= 0x000000 && address <= 0x0000ff) {
    if(address == 0x70 && upper) io.vectorLevel4.byte(3) = data.byte(1);
    if(address == 0x70 && lower) io.vectorLevel4.byte(2) = data.byte(0);
    if(address == 0x72 && upper) io.vectorLevel4.byte(1) = data.byte(1);
    if(address == 0x72 && lower) io.vectorLevel4.byte(0) = data.byte(0);
    return;
  }

  if(address >= 0x840000 && address <= 0x85ffff) {
    if(vdp.framebufferAccess) return;
    if(vdp.framebufferEngaged()) { debug(unusual, "[32X FB] 68k write while FEN==1"); return; } // wait instead?
    if(!data && (!upper || !lower)) return;  //8-bit 0x00 writes do not go through
    shm.debugger.tracer.instruction->invalidate(0x0400'0000 | address & 0x1fffe);
    shs.debugger.tracer.instruction->invalidate(0x0400'0000 | address & 0x1fffe);
    if(upper) vdp.bbram[address >> 1 & 0xffff].byte(1) = data.byte(1);
    if(lower) vdp.bbram[address >> 1 & 0xffff].byte(0) = data.byte(0);
    return;
  }

  if(address >= 0x860000 && address <= 0x87ffff) {
    if(vdp.framebufferAccess) return;
    if(vdp.framebufferEngaged()) { debug(unusual, "[32X FB] 68k overwrite while FEN==1"); return; } // wait instead?
    shm.debugger.tracer.instruction->invalidate(0x0402'0000 | address & 0x1fffe);
    shs.debugger.tracer.instruction->invalidate(0x0402'0000 | address & 0x1fffe);
    if(upper && data.byte(1)) vdp.bbram[address >> 1 & 0xffff].byte(1) = data.byte(1);
    if(lower && data.byte(0)) vdp.bbram[address >> 1 & 0xffff].byte(0) = data.byte(0);
    return;
  }

  if(address >= 0x900000 && address <= 0x9fffff) {
    if(!dreq.vram) {
      address = io.romBank * 0x100000 | address & 0x0fffff;
      return cartridge.child->write(upper, lower, address, data);
    }
  }
}
