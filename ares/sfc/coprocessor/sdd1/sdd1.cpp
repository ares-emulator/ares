SDD1 sdd1;
#include "decompressor.cpp"
#include "serialization.cpp"

auto SDD1::unload() -> void {
  rom.reset();
}

auto SDD1::power() -> void {
  //hook S-CPU DMA MMIO registers to gather information for struct dma[];
  //buffer address and transfer size information for use in SDD1::mcuRead()
  bus.map({&SDD1::dmaRead, &sdd1}, {&SDD1::dmaWrite, &sdd1}, "00-3f,80-bf:4300-437f");

  r4800 = 0x00;
  r4801 = 0x00;
  r4804 = 0x00;
  r4805 = 0x01;
  r4806 = 0x02;
  r4807 = 0x03;

  for(auto& channel : dma) {
    channel.address = 0;
    channel.size = 0;
  }

  dmaReady = false;
}

auto SDD1::ioRead(uint24 address, uint8 data) -> uint8 {
  address = 0x4800 | address.bit(0,3);

  switch(address) {
  case 0x4800: return r4800;
  case 0x4801: return r4801;
  case 0x4804: return r4804;
  case 0x4805: return r4805;
  case 0x4806: return r4806;
  case 0x4807: return r4807;
  }

  //00-3f,80-bf:4802-4803,4808-480f falls through to ROM
  return rom.read(address);
}

auto SDD1::ioWrite(uint24 address, uint8 data) -> void {
  address = 0x4800 | address.bit(0,3);

  switch(address) {
  case 0x4800: r4800 = data; break;
  case 0x4801: r4801 = data; break;
  case 0x4804: r4804 = data & 0x8f; break;
  case 0x4805: r4805 = data & 0x8f; break;
  case 0x4806: r4806 = data & 0x8f; break;
  case 0x4807: r4807 = data & 0x8f; break;
  }
}

auto SDD1::dmaRead(uint24 address, uint8 data) -> uint8 {
  return cpu.readDMA(address, data);
}

auto SDD1::dmaWrite(uint24 address, uint8 data) -> void {
  uint channel = address.bit(4,6);
  switch(address.bit(0,3)) {
  case 2: dma[channel].address.byte(0) = data; break;
  case 3: dma[channel].address.byte(1) = data; break;
  case 4: dma[channel].address.byte(2) = data; break;
  case 5: dma[channel].size.byte(0) = data; break;
  case 6: dma[channel].size.byte(1) = data; break;
  }
  return cpu.writeDMA(address, data);
}

auto SDD1::mmcRead(uint24 address) -> uint8 {
  switch(address.bit(20,21)) {
  case 0: return rom.read((uint4)r4804 << 20 | (uint20)address);  //c0-cf:0000-ffff
  case 1: return rom.read((uint4)r4805 << 20 | (uint20)address);  //d0-df:0000-ffff
  case 2: return rom.read((uint4)r4806 << 20 | (uint20)address);  //e0-ef:0000-ffff
  case 3: return rom.read((uint4)r4807 << 20 | (uint20)address);  //f0-ff:0000-ffff
  }
  unreachable;
}

//map address=00-3f,80-bf:8000-ffff
//map address=c0-ff:0000-ffff
auto SDD1::mcuRead(uint24 address, uint8 data) -> uint8 {
  //map address=00-3f,80-bf:8000-ffff
  if(!address.bit(22)) {
    if(!address.bit(23) && address.bit(21) && r4805.bit(7)) address.bit(21) = 0;  //20-3f:8000-ffff
    if( address.bit(23) && address.bit(21) && r4807.bit(7)) address.bit(21) = 0;  //a0-bf:8000-ffff
    address = address.bit(16,21) << 15 | (uint15)address;
    return rom.read(address);
  }

  //map address=c0-ff:0000-ffff
  if(r4800 & r4801) {
    //at least one channel has S-DD1 decompression enabled ...
    for(uint n : range(8)) {
      if(r4800.bit(n) && r4801.bit(n)) {
        //S-DD1 always uses fixed transfer mode, so address will not change during transfer
        if(address == dma[n].address) {
          if(!dmaReady) {
            //prepare streaming decompression
            decompressor.init(address);
            dmaReady = true;
          }

          //fetch a decompressed byte; once finished, disable channel and invalidate buffer
          data = decompressor.read();
          if(--dma[n].size == 0) {
            dmaReady = false;
            r4801.bit(n) = 0;
          }

          return data;
        }  //address matched
      }  //channel enabled
    }  //channel loop
  }  //S-DD1 decompressor enabled

  //S-DD1 decompression mode inactive; return ROM data
  return mmcRead(address);
}

auto SDD1::mcuWrite(uint24 address, uint8 data) -> void {
}
