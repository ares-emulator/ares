#include <ws/ws.hpp>

namespace ares::WonderSwan {

InternalRAM iram;
Bus bus;

auto InternalRAM::power() -> void {
  for(auto& byte : memory) byte = 0x00;
  size = SoC::ASWAN() ? 16_KiB : 64_KiB;
  maskByte = size - 1;
  maskWord = size - 2;
  maskLong = size - 4;
}

auto InternalRAM::serialize(serializer& s) -> void {
  s(array_span<u8>{memory, SoC::ASWAN() ? 16_KiB : 64_KiB});
}

auto Bus::power() -> void {
  for(auto& io : port) io = nullptr;
}

auto Bus::read(n20 address) -> n8 {
  if(!cpu.io.cartridgeEnable && address >= 0x100000 - system.bootROM.size()) {
    return system.bootROM.read(address);
  }
  switch(address.bit(16,19)) { default:
  case 0x0: return iram.read(address);
  case 0x1: return cartridge.readRAM(address);
  case 0x2 ... 0xf: return cartridge.readROM(address);
  }
}

auto Bus::write(n20 address, n8 data) -> void {
  if(!cpu.io.cartridgeEnable && address >= 0x100000 - system.bootROM.size()) {
    return system.bootROM.write(address, data);
  }
  switch(address.bit(16,19)) { default:
  case 0x0: return iram.write(address, data);
  case 0x1: return cartridge.writeRAM(address, data);
  case 0x2 ... 0xf: return cartridge.writeROM(address, data);
  }
}

auto Bus::map(IO* io, u16 lo, maybe<u16> hi) -> void {
  for(u32 address = lo; address <= (hi ? hi() : lo); address++) port[address] = io;
}

auto Bus::readIO(n16 address) -> n8 {
  if(auto io = port[address]) return io->readIO(address);
  if(address == 0x00ca) return 0x80;  //Mama Mitte (unknown status bit)
  return 0x00;
}

auto Bus::writeIO(n16 address, n8 data) -> void {
  if(auto io = port[address]) return io->writeIO(address, data);
}

}
