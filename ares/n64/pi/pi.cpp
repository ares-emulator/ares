#include <n64/n64.hpp>

namespace ares::Nintendo64 {

PI pi;
#include "dma.cpp"
#include "rtc.cpp"
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto PI::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PI");

  if(system._BB()) {
    bb_nand.buffer.allocate((0x200 + 0x10) * 2 + 0xB0 + 0x10);

    bb_nand.nand[0] = &nand0;
    bb_nand.nand[1] = &nand1;
    bb_nand.nand[2] = &nand2;
    bb_nand.nand[3] = &nand3;

    bb_rtc.load();
  }

  debugger.load(node);
}

auto PI::save() -> void {
  if(system._BB())
    bb_rtc.save();
}

auto PI::unload() -> void {
  debugger = {};
  node.reset();

  if(system._BB()) {
    bb_nand.buffer.reset();

    bb_rtc.reset();
  }
}

auto PI::power(bool reset) -> void {
  io = {};
  bsd1 = {};
  bsd2 = {};
  if(system._BB()) {
    bb_gpio = {};
    bb_gpio.power.outputEnable = 1;
    bb_gpio.led.outputEnable = 1;
    bb_gpio.rtc_clock.lineOut = 1;
    bb_gpio.rtc_clock.lineIn = 1;
    bb_gpio.rtc_data.lineOut = 1;
    bb_gpio.rtc_data.lineIn = 1;
    bb_allowed = {};
    bb_ide[0] = {};
    bb_ide[1] = {};
    bb_ide[2] = {};
    bb_ide[3] = {};
    bb_nand.io = {};
    bb_nand.buffer.fill();
    bb_atb = {};
    bb_rtc.ram.fill();
    bb_rtc.stored_linestate = 0b11;
    queue.insert(Queue::BB_RTC_Tick, system.frequency());
  }
}

auto PI::access() -> BBAccess {
  if(mi.secure()) return (BBAccess){
      .buf = 1,
      .flash = 1,
      .atb = 1,
      .aes = 1,
      .dma = 1,
      .gpio = 1,
      .ide = 1,
      .err = 1,
    };
  else return bb_allowed;
}

}
