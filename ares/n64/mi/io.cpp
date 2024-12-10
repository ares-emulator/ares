auto MI::readWord(u32 address_, Thread& thread) -> u32 {
  if(address_ <= 0x043f'ffff) return ioRead(address_);
  n32 address = address_;
  u32 data = 0;
  const char *name = "<None>";

  if(address <= 0x1fc3'ffff) {
    //bootrom/secure ram
    auto read_rom = ~address.bit(17) ^ ~bb_exc.boot_swap;
    auto read_ram =  address.bit(17) ^ ~bb_exc.boot_swap;

    auto fetching = (address == 0x1fc0'0000) && cpu.pipeline.fetch;
    if(fetching && (bb_exc.boot_swap || enter_secure_mode())) bb_exc.secure = 1;

    if(read_rom && secure()) {
      name = "Boot ROM";
      data = rom.read<Word>(address);
    } else if(read_ram && secure()) {
      name = "SK RAM";
      data = ram.read<Word>(address);
    } else {
      debug(unusual, "Read of SK RAM outside of secure mode @ PC=", hex(cpu.ipu.pc, 8L));
    }
  } else if(address <= 0x1fc7'ffff) {
    //scratch ram
    if(secure() | bb_exc.sk_ram_access) {
      name = "Scratch RAM";
      data = scratch.read<Word>(address);
    } else {
      debug(unusual, "Read of secure scratch without access @ PC=", hex(cpu.ipu.pc, 8L));
    }
  }

  if (!cpu.pipeline.fetch)
    debugger.ioMem(Read, address, data, name);
  return data;
}

auto MI::ioRead(u32 address) -> u32 {
  if(system._BB()) address = (address & 0x3f) >> 2;
  else             address = (address & 0x0f) >> 2;
  n32 data;

  if(address == 0) {
    //MI_INIT_MODE
    data.bit(0,6) = io.initializeLength;
    data.bit(7)   = io.initializeMode;
    data.bit(8)   = io.ebusTestMode;
    data.bit(9)   = io.rdramRegisterSelect;
  }

  if(address == 1) {
    //MI_VERSION
    data.byte(0) = revision.io;
    data.byte(1) = revision.rac;
    data.byte(2) = revision.rdp;
    data.byte(3) = revision.rsp;
  }

  if(address == 2) {
    //MI_INTR
    data.bit(0) = irq.sp.line;
    data.bit(1) = irq.si.line;
    data.bit(2) = irq.ai.line;
    data.bit(3) = irq.vi.line;
    data.bit(4) = irq.pi.line;
    data.bit(5) = irq.dp.line;
  }

  if(address == 3) {
    //MI_INTR_MASK
    data.bit(0) = irq.sp.mask;
    data.bit(1) = irq.si.mask;
    data.bit(2) = irq.ai.mask;
    data.bit(3) = irq.vi.mask;
    data.bit(4) = irq.pi.mask;
    data.bit(5) = irq.dp.mask;
  }



  if(address == 4) {
    debug(unimplemented, "[MI::ioRead] Read from MI + 0x10");
  }

  if(address == 5) {
    //MI_BB_SECURE_EXCEPTION
    if(!secure()) {
      //printf("SKC %016llX\n", cpu.ipu.r[CPU::IPU::V0].u64);
      bb_trap.application = 1;
      poll();
    } else {
      data.bit( 0) = bb_exc.secure;
      data.bit( 1) = bb_exc.boot_swap;
      data.bit( 2) = bb_exc.application;
      data.bit( 3) = bb_exc.timer;
      data.bit( 4) = bb_exc.pi_error;
      data.bit( 5) = bb_exc.mi_error;
      data.bit( 6) = bb_exc.button;
      data.bit( 7) = bb_exc.md;
      data.bit(24) = bb_exc.sk_ram_access;
    }
  }

  if(address == 6) {
    data.bit(16,31) = u16(bb_timer.count);
    data.bit(0,15) = bb_timer.countStore;
  }

  if(address == 7) {
    debug(unimplemented, "[MI::ioRead] Read from MI + 0x1C");
  }

  if(address == 8) {
    debug(unimplemented, "[MI::ioRead] Read from MI + 0x20");
  }

  if(address == 9) {
    debug(unimplemented, "[MI::ioRead] Read from MI + 0x24");
  }

  if(address == 10) {
    debug(unimplemented, "[MI::ioRead] Read from MI + 0x28");
  }

  if(address == 11) {
    //MI_BB_RANDOM
    data.bit(0) = random();
  }

  if(address == 12) {
    debug(unimplemented, "[MI::ioRead] Read from MI + 0x30");
  }

  if(address == 13) {
    debug(unimplemented, "[MI::ioWrite] Write to MI + 0x34");
  }

  if(address == 14) {
    //MI_BB_INTERRUPT
    data.bit( 0) = irq.sp.line;
    data.bit( 1) = irq.si.line;
    data.bit( 2) = irq.ai.line;
    data.bit( 3) = irq.vi.line;
    data.bit( 4) = irq.pi.line;
    data.bit( 5) = irq.dp.line;
    data.bit( 6) = bb_irq.flash.line;
    data.bit( 7) = bb_irq.aes.line;
    data.bit( 8) = bb_irq.ide.line;
    data.bit( 9) = bb_irq.pi_err.line;
    data.bit(10) = bb_irq.usb0.line;
    data.bit(11) = bb_irq.usb1.line;
    data.bit(12) = bb_irq.btn.line;
    data.bit(13) = bb_irq.md.line;
    data.bit(24) = bb.button;
    data.bit(25) = bb.card;
  }

  if(address == 15) {
    //MI_BB_MASK
    data.bit( 0) = irq.sp.mask;
    data.bit( 1) = irq.si.mask;
    data.bit( 2) = irq.ai.mask;
    data.bit( 3) = irq.vi.mask;
    data.bit( 4) = irq.pi.mask;
    data.bit( 5) = irq.dp.mask;
    data.bit( 6) = bb_irq.flash.mask;
    data.bit( 7) = bb_irq.aes.mask;
    data.bit( 8) = bb_irq.ide.mask;
    data.bit( 9) = bb_irq.pi_err.mask;
    data.bit(10) = bb_irq.usb0.mask;
    data.bit(11) = bb_irq.usb1.mask;
    data.bit(12) = bb_irq.btn.mask;
    data.bit(13) = bb_irq.md.mask;
  }

  debugger.io(Read, address, data);
  return data;
}

auto MI::writeWord(u32 address_, u32 data, Thread& thread) -> void {
  if(address_ <= 0x043f'ffff) return ioWrite(address_, data);
  n32 address = address_;
  const char *name = "<None>";

  if(address <= 0x1fc3'ffff) {
    //bootrom/secure ram
    auto write_rom = ~address.bit(17) ^ ~bb_exc.boot_swap;
    auto write_ram =  address.bit(17) ^ ~bb_exc.boot_swap;

    if(write_rom & secure()) {
      name = "Boot ROM";
      rom.write<Word>(address, data);
    } else if(write_ram & secure()) {
      name = "SK RAM";
      ram.write<Word>(address, data);
    } else {
      debug(unusual, "Writing SK RAM outside of secure mode @ PC=", hex(cpu.ipu.pc, 8L));
    }
  } else if(address <= 0x1fc7'ffff) {
    //scratch ram
    if(secure() | bb_exc.sk_ram_access) {
      name = "Scratch RAM";
      scratch.write<Word>(address, data);
    } else {
      debug(unusual, "Writing secure scratch without access @ PC=", hex(cpu.ipu.pc, 8L));
    }
  }

  debugger.ioMem(Write, address, data, name);
}

auto MI::ioWrite(u32 address, u32 data_) -> void {
  if(system._BB()) address = (address & 0x3f) >> 2;
  else             address = (address & 0x0f) >> 2;
  n32 data = data_;

  if(address == 0) {
    //MI_INIT_MODE
    io.initializeLength = data.bit(0,6);
    if(data.bit( 7)) io.initializeMode = 0;
    if(data.bit( 8)) io.initializeMode = 1;
    if(data.bit( 9)) io.ebusTestMode = 0;
    if(data.bit(10)) io.ebusTestMode = 1;
    if(data.bit(11)) mi.lower(MI::IRQ::DP);
    if(data.bit(12)) io.rdramRegisterSelect = 0;
    if(data.bit(13)) io.rdramRegisterSelect = 1;

    if(io.initializeMode) debug(unimplemented, "[MI::writeWord] initializeMode=1");
    if(io.ebusTestMode  ) debug(unimplemented, "[MI::writeWord] ebusTestMode=1");
  }

  if(address == 1) {
    //MI_VERSION (read-only)
  }

  if(address == 2) {
    //MI_INTR (read-only)
  }

  if(address == 3) {
    //MI_INTR_MASK
    if(data.bit( 0)) irq.sp.mask = 0;
    if(data.bit( 1)) irq.sp.mask = 1;
    if(data.bit( 2)) irq.si.mask = 0;
    if(data.bit( 3)) irq.si.mask = 1;
    if(data.bit( 4)) irq.ai.mask = 0;
    if(data.bit( 5)) irq.ai.mask = 1;
    if(data.bit( 6)) irq.vi.mask = 0;
    if(data.bit( 7)) irq.vi.mask = 1;
    if(data.bit( 8)) irq.pi.mask = 0;
    if(data.bit( 9)) irq.pi.mask = 1;
    if(data.bit(10)) irq.dp.mask = 0;
    if(data.bit(11)) irq.dp.mask = 1;
    poll();
  }



  if(address == 4) {
    debug(unimplemented, "[MI::ioWrite] Write to MI + 0x10");
  }

  if(address == 5) {
    //MI_BB_SECURE_EXCEPTION
    if(secure()) {
      bb_exc.secure        = data.bit( 0);
      bb_exc.boot_swap     = data.bit( 1);
      bb_exc.application   = data.bit( 2) ? bb_exc.application : bb_trap.application;
      bb_exc.timer         = data.bit( 3) ? bb_exc.timer       : bb_trap.timer;
      bb_exc.pi_error      = data.bit( 4) ? bb_exc.pi_error    : bb_trap.pi_error;
      bb_exc.mi_error      = data.bit( 5) ? bb_exc.mi_error    : bb_trap.mi_error;
      bb_exc.button        = data.bit( 6) ? bb_exc.button      : bb_trap.button;
      bb_exc.md            = data.bit( 7) ? bb_exc.md          : bb_trap.md;
      bb_exc.sk_ram_access = data.bit(24);
      poll();
      if (!bb_exc.secure) {
        if constexpr(Accuracy::CPU::Recompiler) {
          cpu.recompiler.invalidateRange(0x1fc0'0000, 0x80000);
        }
        //printf("Exit secure mode @ PC=%016llX\n", cpu.ipu.pc);
      }
    }
  }

  if(address == 6) {
    bb_timer.rateStore = data.bit(16,31);
    bb_timer.countStore = data.bit(0,15);
    bb_timer.written = 1;
  }

  if(address == 7) {
    debug(unimplemented, "[MI::ioWrite] Write to MI + 0x1C");
  }

  if(address == 8) {
    debug(unimplemented, "[MI::ioWrite] Write to MI + 0x20");
  }

  if(address == 9) {
    debug(unimplemented, "[MI::ioWrite] Write to MI + 0x24");
  }

  if(address == 10) {
    debug(unimplemented, "[MI::ioWrite] Write to MI + 0x28");
  }

  if(address == 11) {
    //MI_BB_RANDOM (read-only)
    debug(unusual, "[MI::ioWrite] Write to MI_BB_RANDOM");
  }

  if(address == 12) {
    debug(unimplemented, "[MI::ioWrite] Write to MI + 0x30");
  }

  if(address == 13) {
    debug(unimplemented, "[MI::ioWrite] Write to MI + 0x34");
  }

  if(address == 14) {
    //MI_BB_INTERRUPT (read-only)
    debug(unusual, "[MI::ioWrite] Write to MI_BB_INTERRUPT");
  }

  if(address == 15) {
    //MI_BB_MASK
    if(data.bit( 0)) irq.sp.mask = 0;
    if(data.bit( 1)) irq.sp.mask = 1;
    if(data.bit( 2)) irq.si.mask = 0;
    if(data.bit( 3)) irq.si.mask = 1;
    if(data.bit( 4)) irq.ai.mask = 0;
    if(data.bit( 5)) irq.ai.mask = 1;
    if(data.bit( 6)) irq.vi.mask = 0;
    if(data.bit( 7)) irq.vi.mask = 1;
    if(data.bit( 8)) irq.pi.mask = 0;
    if(data.bit( 9)) irq.pi.mask = 1;
    if(data.bit(10)) irq.dp.mask = 0;
    if(data.bit(11)) irq.dp.mask = 1;
    if(data.bit(12)) bb_irq.flash .mask = 0;
    if(data.bit(13)) bb_irq.flash .mask = 1;
    if(data.bit(14)) bb_irq.aes   .mask = 0;
    if(data.bit(15)) bb_irq.aes   .mask = 1;
    if(data.bit(16)) bb_irq.ide   .mask = 0;
    if(data.bit(17)) bb_irq.ide   .mask = 1;
    if(data.bit(18)) bb_irq.pi_err.mask = 0;
    if(data.bit(19)) bb_irq.pi_err.mask = 1;
    if(data.bit(20)) bb_irq.usb0  .mask = 0;
    if(data.bit(21)) bb_irq.usb0  .mask = 1;
    if(data.bit(22)) bb_irq.usb1  .mask = 0;
    if(data.bit(23)) bb_irq.usb1  .mask = 1;
    if(data.bit(24)) bb_irq.btn   .mask = 0;
    if(data.bit(25)) bb_irq.btn   .mask = 1;
    if(data.bit(26)) bb_irq.md    .mask = 0;
    if(data.bit(27)) bb_irq.md    .mask = 1;
  }

  debugger.io(Write, address, data);
}
