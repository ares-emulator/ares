#include <ngp/ngp.hpp>

namespace ares::NeoGeoPocket {

CPU cpu;
#include "memory.cpp"
#include "io.cpp"
#include "ports.cpp"
#include "interrupts.cpp"
#include "timers.cpp"
#include "adc.cpp"
#include "rtc.cpp"
#include "watchdog.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto CPU::load(Node::Object parent) -> void {
  ram.allocate(12_KiB, 0x00);

  if(auto fp = platform->open(system.node, "cpu.ram", File::Read)) {
    ram.load(fp);

    //hack: the BIOS checks (0x6c14) to determine if the setup menu needs to be executed.
    //for unknown reasons, the BIOS from (0xffff00) only writes 0x00dc.
    //it is not updated when language/color choices are made.
    //to prevent the setup from being run every boot, manually calculate and set this value.
    uint16 data;
    data += ram[0x2f87];  //language: Japanese, English
    for(uint address = 0x2c25; address <= 0x2c2b; address++) data += ram[address];  //always 0x00dc
    if(Model::NeoGeoPocketColor()) {
      data += ram[0x2f94];  //K1GE color mode: Black & White, Blue, Green, Red, Classic
    }
    ram[0x2c14] = data.byte(0);
    ram[0x2c15] = data.byte(1);

    //signature check
    ram[0x2e96] = 'N';  //'N'eo Geo
    ram[0x2e95] = 'P';  //'P'ocket

    //this byte seems to indicate system state (0x00 = BIOS UI, 0x10 = setup, 0x40 = game playing)
    //for unknown reasons, sometimes d4 gets set, which re-enters the BIOS setup again.
    ram[0x2f83].bit(4) = 0;

    //this setting gets erased sometimes for unknown reasons, preventing games from booting.
    ram[0x2f91] = Model::NeoGeoPocketColor() ? 0x10 : 0x00;
    ram[0x2f95] = ram[0x2f91];
  }

  node = parent->append<Node::Component>("CPU");

  debugger.load(node);
}

auto CPU::save() -> void {
  if(auto fp = platform->open(system.node, "cpu.ram", File::Write)) {
    ram.save(fp);
  }
}

auto CPU::unload() -> void {
  ram.reset();
  node = {};
  debugger = {};
}

auto CPU::main() -> void {
  if(interrupts.fire()) {
    debugger.interrupt("IRQ");
    r.halted = false;
  }

  if(r.halted) {
    return step(16);
  }

  debugger.instruction();
  instruction();
}

auto CPU::step(uint clocks) -> void {
  clocks <<= clock.rate;
  prescaler.step(clocks);
  adc.step(clocks);
  rtc.step(clocks);
  watchdog.step(clocks);
  Thread::step(clocks);
  Thread::synchronize();
}

auto CPU::idle(uint clocks) -> void {
  clocks <<= clock.rate;
  prescaler.step(clocks);
  adc.step(clocks);
  rtc.step(clocks);
  watchdog.step(clocks);
  Thread::step(clocks);
}

auto CPU::pollPowerButton() -> void {
//platform->input(system.controls.power);
  nmi.set(!system.controls.power->value());
}

auto CPU::power() -> void {
  TLCS900H::power();

  //the CPU runs at 6144000hz, but each TLCS900/H state takes two clock cycles.
  //halve the thread frequency instead of doubling the cycles for every instruction.
  Thread::create(system.frequency() / 2.0, {&CPU::main, this});

  uint24 address;
  address.byte(0) = system.bios.read(0xff00);
  address.byte(1) = system.bios.read(0xff01);
  address.byte(2) = system.bios.read(0xff02);
  //hack: real hardware boots once batteries are connected, and goes into a halt state.
  //power is implemented as a switch rather than a button, so the PC is jumped past
  //this to a hardware reset point, bypassing the need to press controls.power first.
  address = 0xff1800;
  store(PC, address);

  mar = 0;
  mdr = 0;

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

  p10.latch = 1; p10.flow = 1;
  p11.latch = 1; p11.flow = 1;
  p12.latch = 1; p12.flow = 1;
  p13.latch = 1; p13.flow = 1;
  p14.latch = 1; p14.flow = 1;
  p15.latch = 1; p15.flow = 1;
  p16.latch = 1; p16.flow = 1;
  p17.latch = 1; p17.flow = 1;

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
  adc = {};
  rtc = {};
  watchdog = {};

  io = {};
  io.width  = Byte;
  io.timing = 3;
  io.reader = [](uint24 address) -> uint8 { return cpu.readIO(address); };
  io.writer = [](uint24 address, uint8 data) { return cpu.writeIO(address, data); };

  rom = {};
  rom.width  = Word;
  rom.timing = 3;
  rom.reader = [](uint24 address) -> uint8 { return system.bios.read(address); };
  rom.writer = [](uint24 address, uint8 data) { return system.bios.write(address, data); };

  cram = {};
  cram.width  = Word;
  cram.timing = 3;
  cram.reader = [](uint24 address) -> uint8 { return cpu.ram.read(address); };
  cram.writer = [](uint24 address, uint8 data) { return cpu.ram.write(address, data); };

  aram = {};
  aram.width  = Word;
  aram.timing = 3;
  aram.reader = [](uint24 address) -> uint8 { return apu.ram.read(address); };
  aram.writer = [](uint24 address, uint8 data) { return apu.ram.write(address, data); };

  vram = {};
  vram.width  = Word;
  vram.timing = 3;
  vram.reader = [](uint24 address) -> uint8 { return vpu.read(address); };
  vram.writer = [](uint24 address, uint8 data) { return vpu.write(address, data); };

  cs0 = {};
  cs0.width   = Word;
  cs0.timing  = 0;
  cs0.reader  = [](uint24 address) -> uint8 { return cartridge.read(0, address); };
  cs0.writer  = [](uint24 address, uint8 data) { return cartridge.write(0, address, data); };
  cs0.address = 0xff0000;
  cs0.mask    = 0x1fffff;

  cs1 = {};
  cs1.width   = Word;
  cs1.timing  = 0;
  cs1.reader  = [](uint24 address) -> uint8 { return cartridge.read(1, address); };
  cs1.writer  = [](uint24 address, uint8 data) { return cartridge.write(1, address, data); };
  cs1.address = 0xff0000;
  cs1.mask    = 0x3fffff;

  cs2 = {};
  cs2.width   = Word;
  cs2.timing  = 0;
  cs2.reader  = [](uint24 address) -> uint8 { return 0x00; };
  cs2.writer  = [](uint24 address, uint8 data) { return; };
  cs2.address = 0xff0000;
  cs2.mask    = 0x7fffff;
  cs2.mode    = 0;

  cs3 = {};
  cs3.width   = Word;
  cs3.timing  = 0;
  cs3.reader  = [](uint24 address) -> uint8 { return 0x00; };
  cs3.writer  = [](uint24 address, uint8 data) { return; };
  cs3.address = 0xff0000;
  cs3.mask    = 0x7fffff;
  cs3.cas     = 0;

  csx = {};
  csx.width  = Word;
  csx.timing = 0;
  csx.reader = [](uint24 address) -> uint8 { return 0x00; };
  csx.writer = [](uint24 address, uint8 data) { return; };

  clock = {};

  misc = {};
}

//code to skip the BIOS boot sequence and load the game immediately.
//note that this will not configure the language, color mode, or real-time clock.
auto CPU::fastBoot() -> void {
  uint32 address;
  address.byte(0) = cartridge.read(0, 0x1c);
  address.byte(1) = cartridge.read(0, 0x1d);
  address.byte(2) = cartridge.read(0, 0x1e);
  store(PC, address);
  store(XSP, 0x6c00);

  //configure initial system I/O state to values after the BIOS set has completed.
  writeIO(0x20, 0x80);
  writeIO(0x22, 0x01);
  writeIO(0x23, 0x90);
  writeIO(0x24, 0x03);
  writeIO(0x25, 0xfc);
  writeIO(0x26, 0x90);
  writeIO(0x27, 0x62);
  writeIO(0x28, 0x05);
  writeIO(0x38, 0x30);
  writeIO(0x3c, 0x20);
  writeIO(0x3d, 0xff);
  writeIO(0x3e, 0x80);
  writeIO(0x3f, 0x7f);
  writeIO(0x48, 0x30);
  writeIO(0x51, 0x20);
  writeIO(0x52, 0x20);
  writeIO(0x53, 0x15);
  writeIO(0x5c, 0xff);
  writeIO(0x5d, 0xff);
  writeIO(0x5e, 0xff);
  writeIO(0x5f, 0xff);
  writeIO(0x68, 0x17);
  writeIO(0x69, 0x17);
  writeIO(0x6a, 0x03);
  writeIO(0x6b, 0x03);
  writeIO(0x6c, 0x02);
  writeIO(0x6d, 0x04);
  writeIO(0x6e, 0xf0);
  writeIO(0x6f, 0x4e);
  writeIO(0x70, 0x0a);
  writeIO(0x71, 0xdc);
  writeIO(0x72, 0x00);
  writeIO(0x73, 0x00);
  writeIO(0x74, 0x00);
  writeIO(0x77, 0x00);
  writeIO(0x79, 0x00);
  writeIO(0x7a, 0x00);
  writeIO(0x7b, 0x04);
  writeIO(0x80, 0x00);
  writeIO(0xb2, 0x01);
  writeIO(0xb3, 0x04);
  writeIO(0xb4, 0x0a);
  writeIO(0xb5, 0x00);
  writeIO(0xb6, 0x05);
  writeIO(0xb7, 0x00);
  writeIO(0xb8, 0xaa);
  writeIO(0xb9, 0xaa);
  writeIO(0xba, 0xfc);
  writeIO(0xbc, 0x03);

  vpu.write(0x8000, 0xc0);  //enable Vblank and Hblank interrupts

  //default color palette: Magical Drop relies on the BIOS to initialize this.
  for(uint address = 0x8380; address < 0x8400; address++) {
    static uint8 data[16] = {
      0xff, 0x0f, 0xdd, 0x0d, 0xbb, 0x0b, 0x99, 0x09,
      0x77, 0x07, 0x44, 0x04, 0x33, 0x03, 0x00, 0x00,
    };
    vpu.write(address, data[address & 15]);
  }

  ram[0x2c55] = 0x01;  //game type (0x01 = game)
  ram[0x2c58] = cartridge.flash[0] ? 0x03 : 0x00;  //flash type
  ram[0x2c59] = cartridge.flash[1] ? 0x03 : 0x00;  //flash type

  ram[0x2f80] = 0xff;  //battery level (lo)
  ram[0x2f81] = 0x03;  //battery level (hi)
  ram[0x2f84] = 0x40;  //user boot: power on
  ram[0x2f85] = 0x00;  //user shutdown: no
  ram[0x2f86] = 0x00;  //user response: none
  ram[0x2f87] = 0x01;  //language: English
  ram[0x2f91] = Model::NeoGeoPocketColor() ? 0x10 : 0x00;  //hardware model
  ram[0x2f92] = ram[0x2c58];
  ram[0x2f93] = ram[0x2c59];
  ram[0x2f95] = ram[0x2f91];
}

}
