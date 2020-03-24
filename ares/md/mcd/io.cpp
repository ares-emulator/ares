auto MCD::readIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> uint16 {
  address = 0xff8000 | (uint9)address;

  if(address == 0xff8000) {
    data.bit(0)     = 1;  //peripheral ready
    data.bit(1,3)   = Unmapped;
    data.bit(4,7)   = 0b0000;  //version#
    data.bit(8)     = led.red;
    data.bit(9)     = led.green;
    data.bit(10,15) = Unmapped;
  }

  if(address == 0xff8002) {
    data.bit(0)    =!io.wramMode ? !io.wramSwitch : +io.wramSelect;
    data.bit(1)    = io.wramSwitch;
    data.bit(2)    = io.wramMode;
    data.bit(3, 4) = io.wramPriority;
    data.bit(5, 7) = Unmapped;
    data.bit(8,15) = io.pramProtect;
  }

  if(address == 0xff8004) {
    data.bit(0,3)   = cdc.address;
    data.bit(4,7)   = Unmapped;
    data.bit( 8,10) = cdc.transfer.destination;
    data.bit(11,13) = Unmapped;
    data.bit(14)    = cdc.transfer.ready;
    data.bit(15)    = cdc.transfer.completed;
  }

  if(address == 0xff8006) {
    if(lower) {  //unconfirmed
      data.bit(0, 7) = cdc.read();
    }
    if(upper) {
      data.bit(8,15) = Unmapped;
    }
  }

  if(address == 0xff8008) {
    if(!upper || !lower) print("* read ff8008 (byte)\n");
    data = cdc.transfer.read();
  }

  if(address == 0xff800a) {
    data.bit(0,15) = cdc.transfer.address.bit(3,18);
  }

  if(address == 0xff800c) {
    data.bit( 0,11) = cdc.stopwatch;
    data.bit(12,15) = Unmapped;
  }

  if(address == 0xff800e) {
    data.byte(0) = communication.cfs;
    data.byte(1) = communication.cfm;
  }

  if(address >= 0xff8010 && address <= 0xff801f) {
    uint3 index = address - 0xff8010 >> 1;
    data = communication.command[index];
  }

  if(address >= 0xff8020 && address <= 0xff802f) {
    uint3 index = address - 0xff8020 >> 1;
    data = communication.status[index];
  }

  if(address == 0xff8030) {
    data.bit(0, 7) = timer.counter;
    data.bit(8,15) = Unmapped;
  }

  if(address == 0xff8032) {
    data.bit(0)    = Unmapped;
    data.bit(1)    = gpu.irq.enable;
    data.bit(2)    = external.irq.enable;
    data.bit(3)    = timer.irq.enable;
    data.bit(4)    = cdd.irq.enable;
    data.bit(5)    = cdc.irq.enable;
    data.bit(6)    = irq.subcode.enable;
    data.bit(7,15) = Unmapped;
  }

  if(address == 0xff8034) {
    data.bit(0,14) = Unmapped;
    data.bit(15)   = 0;  //end of fade data transfer
  }

  if(address == 0xff8036) {
    data.bit(0)    = 0;  //1 = sending command to CDD
    data.bit(1)    = 0;  //1 = sending status from CDD
    data.bit(2)    = cdd.hostClockEnable;
    data.bit(3, 7) = Unmapped;
    data.bit(8)    = cdd.session.tracks[cdd.io.track].isData();
    data.bit(9,15) = Unmapped;
  }

  if(address >= 0xff8038 && address <= 0xff8041) {
    uint index = address - 0xff8038;
    data.bit( 0, 3) = cdd.status[index | 1];
    data.bit( 4, 7) = Unmapped;
    data.bit( 8,11) = cdd.status[index | 0];
    data.bit(12,15) = Unmapped;
  }

  if(address >= 0xff8042 && address <= 0xff804b) {
    uint index = address - 0xff8042;
    data.bit( 0, 3) = cdd.command[index | 1];
    data.bit( 4, 7) = Unmapped;
    data.bit( 8,11) = cdd.command[index | 0];
    data.bit(12,15) = Unmapped;
  }

  if(address == 0xff804c) {
    data.bit(0, 3) = gpu.font.color.background;
    data.bit(4, 7) = gpu.font.color.foreground;
    data.bit(8,15) = Unmapped;
  }

  if(address == 0xff804e) {
    data = gpu.font.data;
  }

  if(address >= 0xff8050 && address <= 0xff8057) {
    uint index = 12 - (address - 0xff8050 << 1);
    auto foreground = gpu.font.color.foreground;
    auto background = gpu.font.color.background;
    data.bit( 0, 3) = gpu.font.data.bit(index | 0) ? foreground : background;
    data.bit( 4, 7) = gpu.font.data.bit(index | 1) ? foreground : background;
    data.bit( 8,11) = gpu.font.data.bit(index | 2) ? foreground : background;
    data.bit(12,15) = gpu.font.data.bit(index | 3) ? foreground : background;
  }

  if(address == 0xff8058) {
    data.bit( 0)    = gpu.stamp.repeat;
    data.bit( 1)    = gpu.stamp.tile.size;
    data.bit( 2)    = gpu.stamp.map.size;
    data.bit( 3,14) = Unmapped;
    data.bit(15)    = gpu.active;
  }

  if(address == 0xff805a) {
    data.bit(0, 6) = Unmapped;
    data.bit(7,15) = gpu.stamp.map.base.bit(9,17);
  }

  if(address == 0xff805c) {
    data.bit(0, 4) = gpu.image.vcells;
    data.bit(5,15) = Unmapped;
  }

  if(address == 0xff805e) {
    data.bit(0, 2) = Unmapped;
    data.bit(3,15) = gpu.image.base.bit(5,17);
  }

  if(address == 0xff8060) {
    data.bit(0, 5) = gpu.image.offset;
    data.bit(6,15) = Unmapped;
  }

  if(address == 0xff8062) {
    data.bit(0, 8) = gpu.image.hdots;
    data.bit(9,15) = Unmapped;
  }

  if(address == 0xff8064) {
    data.bit(0, 7) = gpu.image.vdots;
    data.bit(8,15) = Unmapped;
  }

  //todo: hardware manual says this register is write-only; software manual says it is readable
  if(address == 0xff8066) {
    data.bit(0)    = Unmapped;
    data.bit(1,15) = gpu.vector.base.bit(3,17);
  }

  if(address >= 0xff8100 && address <= 0xff81ff) {
    print("* read ", hex(address, 6L), "\n");
  }

  return data;
}

auto MCD::writeIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> void {
  address = 0xff8000 | (uint9)address;

  if(address == 0xff8000) {
    if(lower) {
      if(data.bit(0) == 0);  //peripheral reset
    }
    if(upper) {
      led.red   = data.bit(8);
      led.green = data.bit(9);
    }
  }

  if(address == 0xff8002) {
    if(lower) {
      io.wramSelect   = data.bit(0);
      io.wramMode     = data.bit(2);
      io.wramPriority = data.bit(3,4);
      io.wramSwitch   = 0;
    }
  }

  if(address == 0xff8004) {
    if(lower) {
      cdc.address = data.bit(0,3);
    }
    if(upper) {
      cdc.transfer.destination = data.bit(0,2);
    }
  }

  if(address == 0xff8006) {
    if(lower) {  //unconfirmed
      cdc.write(data.byte(0));
    }
  }

  if(address == 0xff8008) {
    //read-only
  }

  if(address == 0xff800a) {
    cdc.transfer.address.bit(0, 2) = 0;  //alignment
    cdc.transfer.address.bit(3,18) = data.bit(0,15);
  }

  if(address == 0xff800c) {
    cdc.stopwatch = 0;
  }

  if(address == 0xff800e) {
    if(lower) {  //unconfirmed
      communication.cfs = data.byte(0);
    }
  }

  if(address >= 0xff8010 && address <= 0xff801f) {
    //read-only
  }

  if(address >= 0xff8020 && address <= 0xff802f) {
    uint3 index = address - 0xff8020 >> 1;
    if(lower) communication.status[index].byte(0) = data.byte(0);
    if(upper) communication.status[index].byte(1) = data.byte(1);
  }

  if(address == 0xff8030) {
    if(lower) {
      timer.counter = data.byte(0);
    }
  }

  if(address == 0xff8032) {
    if(lower) {
      gpu.irq.enable      = data.bit(1);
      external.irq.enable = data.bit(2);
      timer.irq.enable    = data.bit(3);
      cdd.irq.enable      = data.bit(4);
      cdc.irq.enable      = data.bit(5);
      irq.subcode.enable  = data.bit(6);
      //todo: does disabling an interrupt clear its pending status if it were triggered already?
    }
  }

  if(address == 0xff8034) {
    cdd.dac.rate = data.bit(1);
    cdd.dac.deemphasis = data.bit(2,3);
    cdd.dac.attenuator.bit(6,15) = data.bit(6,15);
    cdd.dac.reconfigure();
  }

  if(address == 0xff8036) {
    if(lower) {
      if(!cdd.hostClockEnable && data.bit(2)) cdd.irq.raise();
      cdd.hostClockEnable = data.bit(2);
      cdd.counter = 0;
    }
  }

  if(address >= 0xff8038 && address <= 0xff8041) {
    //read-only
    print("* write ", hex(address, 6L), "\n");
  }

  if(address >= 0xff8042 && address <= 0xff804b) {
    uint index = address - 0xff8042;
    if(lower) cdd.command[index | 1] = data.bit(0, 3);
    if(upper) cdd.command[index | 0] = data.bit(8,11);
    if(lower && (index | 1) == 9) {  //unconfirmed
      cdd.process();
    }
  }

  if(address == 0xff804c) {
    if(lower) {
      gpu.font.color.background = data.bit(0,3);
      gpu.font.color.foreground = data.bit(4,7);
    }
  }

  if(address == 0xff804e) {
    if(lower) gpu.font.data.byte(0) = data.byte(0);
    if(upper) gpu.font.data.byte(1) = data.byte(1);
  }

  if(address >= 0xff8050 && address <= 0xff8057) {
    //read-only
  }

  if(address == 0xff8058) {
    if(lower) {
      gpu.stamp.repeat    = data.bit(0);
      gpu.stamp.tile.size = data.bit(1);
      gpu.stamp.map.size  = data.bit(2);
    }
  }

  if(address == 0xff805a) {
    gpu.stamp.map.base.bit(7,17) = data.bit(5,15);
  }

  if(address == 0xff805c) {
    gpu.image.vcells = data.bit(0,4);
  }

  if(address == 0xff805e) {
    gpu.image.base.bit(5,17) = data.bit(3,15);
  }

  if(address == 0xff8060) {
    gpu.image.offset = data.bit(0,5);
  }

  if(address == 0xff8062) {
    gpu.image.hdots = data.bit(0,8);
  }

  if(address == 0xff8064) {
    gpu.image.vdots = data.bit(0,7);
  }

  if(address == 0xff8066) {
    gpu.vector.base.bit(3,17) = data.bit(1,15);
    gpu.start();
  }

  if(address >= 0xff8100 && address <= 0xff81ff) {
    print("* write ", hex(address, 6L), "=", hex(data, 4L), "\n");
  }
}
