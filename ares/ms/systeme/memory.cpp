#include <ms/ms.hpp>

namespace ares::MasterSystem::SystemE {

Memory memory;

auto Memory::read(n16 address) -> n8 {
  if(address < 0xc000) return cartridge.read(address, 0xff);
  return cpu.ram.read(address);
}

auto Memory::write(n16 address, n8 data) -> void {
  if(address >= 0xc000) cpu.ram.write(address, data);
  else if(address >= 0x8000) cartridge.write(address, data);
}

auto Memory::in(n16 address) -> n8 {
  n8 data = 0xff;
  switch(address & 0xff) {
  case 0x7e: data = vdp2.vcounterQuery(); break;
  case 0x7f: data = vdp.hcounterQuery(); break;
  case 0xba: data = vdp2.data(); break;
  case 0xbb: data = vdp2.status(); break;
  case 0xbe: data = vdp.data(); break;
  case 0xbf: data = vdp.status(); break;
  case 0xe0: data = system.arcadeControls.coinPort(); break;
  case 0xe1: data = system.arcadeControls.playerPort(0); break;
  case 0xe2: data = system.arcadeControls.playerPort(1); break;
  case 0xf2: data = system.dips.read("f2"); break;
  case 0xf3: data = system.dips.read("f3"); break;
  case 0xf8: case 0xf9: case 0xfa: case 0xfb:
    data = ppi.read(address & 3);
    break;
  }
  cpu.debugger.in(address, data);
  return data;
}

auto Memory::out(n16 address, n8 data) -> void {
  switch(address & 0xff) {
  case 0x7b: case 0x7e: case 0x7f: psg.write(data); break;
  case 0xba: vdp2.data(data); break;
  case 0xbb: vdp2.control(data); break;
  case 0xbe: vdp.data(data); break;
  case 0xbf: vdp.control(data); break;
  case 0xf7:
    cartridge.out(address, data);
    break;
  case 0xf8: case 0xf9: case 0xfa: case 0xfb:
    ppi.write(address & 3, data);
    break;
  }
  cpu.debugger.out(address, data);
}

}
