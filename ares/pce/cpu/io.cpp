auto CPU::read(uint8 bank, uint13 address) -> uint8 {
  uint8 data = 0xff;

  //$00-7f  HuCard or PC Engine Duo
  //$80-87  CD WRAM
  //$f7     CD BRAM
  if(PCD::Present()) data = pcd.read(bank, address, data);
  if(cartridge.node) data = cartridge.read(bank, address, data);

  //$f8-fb  RAM
  if(bank >= 0xf8 && bank <= 0xfb) {
    return ram.read(bank.bit(0,1) << 13 | address);
  }

  //$ff  Hardware
  if(bank == 0xff) {
    //$0000-03ff  VDC or VPC
    if((address & 0x1c00) == 0x0000) {
      if(Model::SuperGrafx() == 0) return vdp.vdc0.read(address);
      if(Model::SuperGrafx() == 1) return vdp.vpc.read(address);
    }

    //$0400-07ff  VCE
    if((address & 0x1c00) == 0x0400) {
      return vdp.vce.read(address);
    }

    //$0800-0bff  PSG
    if((address & 0x1c00) == 0x0800) {
      if(HuC6280::blockMove) return 0x00;
      return io.buffer;
    }

    //$0c00-0fff  Timer
    if((address & 0x1c00) == 0x0c00) {
      if(HuC6280::blockMove) return 0x00;
      data.bit(0,6) = timer.value;
      data.bit(7)   = io.buffer.bit(7);
      return data;
    }

    //$1000-13ff  I/O
    if((address & 0x1c00) == 0x1000) {
      if(HuC6280::blockMove) return 0x00;
      //note 1: Turbografx-16 games check this bit for region protection.
      //yet PC Engine games do not. since we cannot tell the games apart,
      //it's more compatible to always identify as a Turbografx-16 system.
      //note 2: we state that the CD-ROM drive is present.
      //this is so games can use its backup RAM for save data.
      data.bit(0,3) = controllerPort.read();
      data.bit(4)   = 1;
      data.bit(5)   = 1;
      data.bit(6)   = 0;  //device (0 = Turbografx-16; 1 = PC Engine)
      data.bit(7)   = PCD::Present() == 0;  //add-on (0 = CD-ROM; 1 = nothing)
      return data;
    }

    //$1400-17ff  IRQ
    if((address & 0x1c00) == 0x1400) {
      if(HuC6280::blockMove) return 0x00;

      if(address.bit(0,1) == 0) {
        return io.buffer;
      }

      if(address.bit(0,1) == 1) {
        return io.buffer;
      }

      if(address.bit(0,1) == 2) {
        data.bit(0)   = irq2.disable;
        data.bit(1)   = irq1.disable;
        data.bit(2)   = tiq.disable;
        data.bit(3,7) = io.buffer.bit(3,7);
        return data;
      }

      if(address.bit(0,1) == 3) {
        data.bit(0)   = 0;  //IRQ2 line
        data.bit(1)   = vdp.irqLine();
        data.bit(2)   = timer.irqLine();
        data.bit(3,7) = io.buffer.bit(3,7);
        return data;
      }
    }

    //$1800-1bff  CD I/O
    if((address & 0x1c00) == 0x1800) {
      if(PCD::Present()) return pcd.readIO(address, data);
      return data;
    }

    //$1c00-1fff  unmapped
    if((address & 0x1c00) == 0x1c00) {
      return data;
    }
  }

  return data;
}

auto CPU::write(uint8 bank, uint13 address, uint8 data) -> void {
  //$00-7f  HuCard or PC Engine Duo
  //$80-87  CD WRAM
  //$f7     CD BRAM
  if(PCD::Present()) pcd.write(bank, address, data);
  if(cartridge.node) cartridge.write(bank, address, data);

  //$f8-fb  RAM
  if(bank >= 0xf8 && bank <= 0xfb) {
    ram.write(bank.bit(0,1) << 13 | address, data);
    return;
  }

  //$ff  Hardware
  if(bank == 0xff) {
    //$0000-03ff  VDC or VPC
    if((address & 0x1c00) == 0x0000) {
      if(Model::SuperGrafx() == 0) return vdp.vdc0.write(address, data);
      if(Model::SuperGrafx() == 1) return vdp.vpc.write(address, data);
    }

    //$0400-07ff  VCE
    if((address & 0x1c00) == 0x0400) {
      return vdp.vce.write(address, data);
    }

    //$0800-0bff  PSG
    if((address & 0x1c00) == 0x0800) {
      synchronize(psg);
      io.buffer = data;
      return psg.write(address, data);
    }

    //$0c00-0fff  Timer
    if((address & 0x1c00) == 0x0c00) {
      io.buffer = data;
      if(!address.bit(0)) {
        timer.reload = data.bit(0,6);
      } else {
        if(!timer.enable && data.bit(0)) {
          timer.value = timer.reload;
          timer.counter = 1024 * 3;
        }
        timer.enable = data.bit(0);
      }
      return;
    }

    //$1000-13ff  I/O
    if((address & 0x1c00) == 0x1000) {
      io.buffer = data;
      controllerPort.write(data.bit(0,1));
      return;
    }

    //$1400-17ff  IRQ
    if((address & 0x1c00) == 0x1400) {
      io.buffer = data;
      if(address.bit(0,1) == 2) {
        irq2.disable = data.bit(0);
        irq1.disable = data.bit(1);
        tiq.disable  = data.bit(2);
        return;
      }

      if(address.bit(0,1) == 3) {
        timer.line = 0;
        return;
      }
    }

    //$1800-1bff  CD I/O
    if((address & 0x1c00) == 0x1800) {
      if(PCD::Present()) return pcd.writeIO(address, data);
      return;
    }

    //$1c00-1fff  unmapped
    if((address & 0x1c00) == 0x1c00) {
      return;
    }
  }

  return;
}

//ST0, ST1, ST2
auto CPU::store(uint2 address, uint8 data) -> void {
  if(address) address++;  //0,1,2 => 0,2,3
  if(Model::SuperGrafx() == 0) vdp.vdc0.write(address, data);
  if(Model::SuperGrafx() == 1) vdp.vpc.store(address, data);
}
