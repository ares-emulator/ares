#include <ws/ws.hpp>

namespace ares::WonderSwan {

InternalRAM iram;
Bus bus;

auto InternalRAM::power() -> void {
  for(auto& byte : memory) byte = 0x00;
}

auto InternalRAM::serialize(serializer& s) -> void {
  s(array_span<u8>{memory, SoC::ASWAN() ? 16_KiB : 64_KiB});
}

auto InternalRAM::read(n16 address) -> n8 {
  if(address >= system.memory()) return 0x90;
  return memory[address];
}

auto InternalRAM::write(n16 address, n8 data) -> void {
  if(address >= system.memory()) return;
  memory[address] = data;
}

auto Bus::power() -> void {
  for(auto& io : port) io = nullptr;
}

auto Bus::read(n20 address) -> n8 {
  if(!cpu.r.cartridgeEnable && address >= 0x100000 - system.bootROM.size()) {
    return system.bootROM.read(address);
  }
  n4 bank = address.bit(16,19);
  if(bank == 0x0) return iram.read(address);
  if(bank == 0x1) return cartridge.ramRead(address);
  if(bank >= 0x2) return cartridge.romRead(address);
  return 0x00;
}

auto Bus::write(n20 address, n8 data) -> void {
  if(!cpu.r.cartridgeEnable && address >= 0x100000 - system.bootROM.size()) {
    return system.bootROM.write(address, data);
  }
  n4 bank = address.bit(16,19);
  if(bank == 0x0) return iram.write(address, data);
  if(bank == 0x1) return cartridge.ramWrite(address, data);
  if(bank >= 0x2) return cartridge.romWrite(address, data);
}

auto Bus::map(IO* io, u16 lo, maybe<u16> hi) -> void {
  for(u32 address = lo; address <= (hi ? hi() : lo); address++) port[address] = io;
}

auto Bus::portRead(n16 address) -> n8 {
  if(auto io = port[address]) return io->portRead(address);
  if(address == 0x00ca) return 0x80;  //Mama Mitte (unknown status bit)
  return 0x00;
}

auto Bus::portWrite(n16 address, n8 data) -> void {
  if(auto io = port[address]) return io->portWrite(address, data);
}

}
