#include <ngp/ngp.hpp>

namespace ares::NeoGeoPocket {

CPU cpu;
#include "memory.cpp"
#include "io.cpp"
#include "ports.cpp"
#include "interrupts.cpp"
#include "timers.cpp"
#include "serial.cpp"
#include "adc.cpp"
#include "rtc.cpp"
#include "watchdog.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  ram.allocate(12_KiB, 0x00);

  if(auto fp = system.pak->read("cpu.ram")) {
    ram.load(fp);
  }

  node = parent->append<Node::Object>("CPU");

  debugger.load(node);
}

auto CPU::save() -> void {
  if(auto fp = system.pak->read("cpu.ram")) {
    ram.save(fp);
  }
}

auto CPU::unload() -> void {
  if(!node) return;
  debugger.unload(node);
  ram.reset();
  node.reset();
}

auto CPU::main() -> void {
  if(interrupts.fire()) return (void)(HALT = 0);
  if(HALT) {
    //real hardware boots once batteries are connected and goes into a halt state.
    //here we bypass the need for the user to press controls.power.
    if(privilegedMode()) { //did this halt occur in the BIOS?
      if(!system.controls.power->value()) {
        system.controls.power->setValue(true); //press power on the user's behalf.
        pollPowerButtonSkipCounter = 6000; //half a second, or ~30 frames at 198 lines/frame
      }
    }
    return step(16);
  }
  debugger.instruction();
  instruction();
}

auto CPU::step(u32 clocks) -> void {
  clocks <<= clock.rate;
  prescaler.step(clocks);
  adc.step(clocks);
  rtc.step(clocks);
  watchdog.step(clocks);
  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::pollPowerButton() -> void {
  //suppress user input when we are simulating a power button press.
  if(pollPowerButtonSkipCounter > 0) --pollPowerButtonSkipCounter;
  else platform->input(system.controls.power);
  nmi.set(!system.controls.power->value());
}

auto CPU::power() -> void {
  TLCS900H::power();
  Thread::create(system.frequency(), {&CPU::main, this});

  n24 address;
  //read hardware reset vector.
  address.byte(0) = system.bios.read(0xff00);
  address.byte(1) = system.bios.read(0xff01);
  address.byte(2) = system.bios.read(0xff02);
  //detect whether the loaded ram data has already been through the boot process.
  //this byte is written when entering or leaving a halt state and is never zero,
  //making it convenient to test.
  if(ram[0x2c7a] != 0) {
    //read VECT_SHUTDOWN; it may seem counterintuitive to jump here on power-on,
    //but it allows the BIOS to perform cleanup that would normally only occur
    //when the user shuts off the device before switching cartridges.
    address.byte(0) = system.bios.read(0xfe00);
    address.byte(1) = system.bios.read(0xfe01);
    address.byte(2) = system.bios.read(0xfe02);
    store(XSP, 0x6c00); //system calls require a stack
  }
  store(PC, address);

  interrupts = {};

  nmi.power(0x20);
  nmi.enable = 0;
  nmi.maskable = 0;
  nmi.priority = 7;
  nmi.line = 1;
  nmi.edge.rising = 0;
  nmi.edge.falling = 1;

  intwd.power(0x24);
  intwd.dmaAllowed = 0;
  intwd.enable = 1;
  intwd.maskable = 0;
  intwd.priority = 7;
  intwd.line = 1;
  intwd.edge.falling = 1;

  int0.power(0x28);
  int4.power(0x2c);
  int5.power(0x30);
  int6.power(0x34);
  int7.power(0x38);
  intt0.power(0x40);
  intt1.power(0x44);
  intt2.power(0x48);
  intt3.power(0x4c);
  inttr4.power(0x50);
  inttr5.power(0x54);
  inttr6.power(0x58);
  inttr7.power(0x5c);
  intrx0.power(0x60);
  inttx0.power(0x64);
  intrx1.power(0x68);
  inttx1.power(0x6c);
  intad.power(0x70);

  inttc0.power(0x74);
  inttc0.dmaAllowed = 0;

  inttc1.power(0x78);
  inttc1.dmaAllowed = 0;

  inttc2.power(0x7c);
  inttc2.dmaAllowed = 0;

  inttc3.power(0x80);
  inttc3.dmaAllowed = 0;

  dma0 = {};
  dma1 = {};
  dma2 = {};
  dma3 = {};

  p10.latch = 0; p10.flow = 1;
  p11.latch = 0; p11.flow = 1;
  p12.latch = 0; p12.flow = 1;
  p13.latch = 0; p13.flow = 1;
  p14.latch = 0; p14.flow = 1;
  p15.latch = 0; p15.flow = 1;
  p16.latch = 0; p16.flow = 1;
  p17.latch = 0; p17.flow = 1;

  p20.latch = 1; p20.mode = 1;
  p21.latch = 1; p21.mode = 1;
  p22.latch = 1; p22.mode = 1;
  p23.latch = 1; p23.mode = 1;
  p24.latch = 1; p24.mode = 1;
  p25.latch = 1; p25.mode = 1;
  p26.latch = 1; p26.mode = 1;
  p27.latch = 1; p27.mode = 1;

  p52.latch = 1; p52.flow = 1; p52.mode = 1;
  p53.latch = 1; p53.flow = 1; p53.mode = 1;
  p54.latch = 1; p54.flow = 1; p54.mode = 1;
  p55.latch = 1; p55.flow = 1; p55.mode = 1;

  p60.latch = 1; p60.mode = 1;
  p61.latch = 1; p61.mode = 1;
  p62.latch = 1; p62.mode = 1;
  p63.latch = 1; p63.mode = 1;
  p64.latch = 1; p64.mode = 1;
  p65.latch = 1; p65.mode = 1;

  p70.latch = 1; p70.mode = 1;
  p71.latch = 1; p71.mode = 1;
  p72.latch = 1; p72.mode = 1;
  p73.latch = 1; p73.mode = 1;
  p74.latch = 1; p74.mode = 1;
  p75.latch = 1; p75.mode = 1;
  p76.latch = 1; p76.mode = 1;
  p77.latch = 1; p77.mode = 1;

  p80.latch = 1; p80.flow = 1; p80.mode = 1;
  p81.latch = 1; p81.flow = 1;
  p82.latch = 1; p82.flow = 1; p82.mode = 1;
  p83.latch = 1; p83.flow = 1; p83.mode = 1;
  p84.latch = 1; p84.flow = 1;
  p85.latch = 1; p85.flow = 1; p85.mode = 1;

  p90.latch = 1;
  p91.latch = 1;
  p92.latch = 1;
  p93.latch = 1;

  pa0.latch = 0; pa0.flow = 1;
  pa1.latch = 1; pa1.flow = 1;
  pa2.latch = 1; pa2.flow = 1; pa2.mode = 1;
  pa3.latch = 1; pa3.flow = 1; pa3.mode = 1;

  pb0.latch = 1; pb0.flow = 1;
  pb1.latch = 1; pb1.flow = 1;
  pb2.latch = 1; pb2.flow = 1; pb2.mode = 1;
  pb3.latch = 1; pb3.flow = 1; pb3.mode = 1;
  pb4.latch = 1; pb4.flow = 1;
  pb5.latch = 1; pb5.flow = 1;
  pb6.latch = 1; pb6.flow = 1; pb6.mode = 1;
  pb7.latch = 1; pb7.flow = 1;

  prescaler = {};
  ti0 = {};
  ti4 = {};
  ti5 = {};
  ti6 = {};
  ti7 = {};
  to1 = {};
  to3 = {};
  to4 = {};
  to5 = {};
  to6 = {};
  to7 = {};
  t0 = {};
  t1 = {};
  ff1 = {};
  t01 = {};
  t2 = {};
  t3 = {};
  ff3 = {};
  t23 = {};
  ff4 = {};
  ff5 = {};
  t4 = {};
  ff6 = {};
  t5 = {};
  pg0 = {};
  pg1 = {};
  sc0 = {};
  sc1 = {};
  adc = {};
  rtc = {};
  watchdog = {};
  dram = {};

  io = {};
  io.width  = Byte;
  io.timing = 3;
  io.reader = [](n24 address) -> n8 { return cpu.readIO(address); };
  io.writer = [](n24 address, n8 data) { return cpu.writeIO(address, data); };

  rom = {};
  rom.width  = Word;
  rom.timing = 3;
  rom.reader = [](n24 address) -> n8 { return system.bios.read(address); };
  rom.writer = [](n24 address, n8 data) { return system.bios.write(address, data); };

  cram = {};
  cram.width  = Word;
  cram.timing = 3;
  cram.reader = [](n24 address) -> n8 { return cpu.ram.read(address); };
  cram.writer = [](n24 address, n8 data) { return cpu.ram.write(address, data); };

  aram = {};
  aram.width  = Word;
  aram.timing = 3;
  aram.reader = [](n24 address) -> n8 { return apu.ram.read(address); };
  aram.writer = [](n24 address, n8 data) { return apu.ram.write(address, data); };

  vram = {};
  vram.width  = Word;
  vram.timing = 3;
  vram.reader = [](n24 address) -> n8 { return kge.read(address); };
  vram.writer = [](n24 address, n8 data) { return kge.write(address, data); };

  cs0 = {};
  cs0.width   = Word;
  cs0.timing  = 0;
  cs0.reader  = [](n24 address) -> n8 { return cartridge.read(0, address); };
  cs0.writer  = [](n24 address, n8 data) { return cartridge.write(0, address, data); };
  cs0.address = 0xff0000;
  cs0.mask    = 0x1fffff;

  cs1 = {};
  cs1.width   = Word;
  cs1.timing  = 0;
  cs1.reader  = [](n24 address) -> n8 { return cartridge.read(1, address); };
  cs1.writer  = [](n24 address, n8 data) { return cartridge.write(1, address, data); };
  cs1.address = 0xff0000;
  cs1.mask    = 0x3fffff;

  cs2 = {};
  cs2.width   = Word;
  cs2.timing  = 0;
  cs2.reader  = [](n24 address) -> n8 { return 0x00; };
  cs2.writer  = [](n24 address, n8 data) { return; };
  cs2.address = 0xff0000;
  cs2.mask    = 0x7fffff;
  cs2.mode    = 0;

  cs3 = {};
  cs3.width   = Word;
  cs3.timing  = 0;
  cs3.reader  = [](n24 address) -> n8 { return 0x00; };
  cs3.writer  = [](n24 address, n8 data) { return; };
  cs3.address = 0xff0000;
  cs3.mask    = 0x7fffff;
  cs3.cas     = 0;

  csx = {};
  csx.width  = Word;
  csx.timing = 0;
  csx.reader = [](n24 address) -> n8 { return 0x00; };
  csx.writer = [](n24 address, n8 data) { return; };

  clock = {};

  misc = {};

  pollPowerButtonSkipCounter = 0;
}

}
