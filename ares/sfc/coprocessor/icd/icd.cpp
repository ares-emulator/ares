ICD icd;

#if defined(CORE_GB)

#include "interface.cpp"
#include "io.cpp"
#include "serialization.cpp"

auto ICD::clockFrequency() const -> double {
  return Frequency ? Frequency : system.cpuFrequency();
}

auto ICD::load(Node::Peripheral parent) -> void {
  GameBoy::superGameBoy = this;
  GameBoy::SuperGameBoyInterface::load((Node::Object&)parent);
  GameBoy::cpu.version->setValue(!Frequency ? "SGB-CPU 01" : "CPU SGB2");
  GameBoy::cpu.version->setLatch();
}

auto ICD::unload() -> void {
  GameBoy::SuperGameBoyInterface::unload();

  cpu.coprocessors.removeByValue(this);
  Thread::destroy();
}

auto ICD::main() -> void {
  if(r6003.bit(7)) {
    GameBoy::system.run();
    Thread::step(GameBoy::system.clocksExecuted());
  } else {  //DMG halted
    GameBoy::apu.stream->sample(0.0, 0.0);
    Thread::step(2);  //two clocks per audio sample
  }
  Thread::synchronize(cpu);
}

auto ICD::power(bool reset) -> void {
  //SGB1 uses CPU oscillator; SGB2 uses dedicated oscillator
  Thread::create(clockFrequency() / 5.0, [&] {
    while(true) {
      scheduler.synchronize();
      main();
    }
  });
  cpu.coprocessors.append(this);

  for(auto& packet : this->packet) packet = {};
  packetSize = 0;

  joypID = 3;
  joypLock = 1;
  pulseLock = 1;
  strobeLock = 0;
  packetLock = 0;
  joypPacket = {};
  packetOffset = 0;
  bitData = 0;
  bitOffset = 0;

  for(auto& n : output) n = 0xff;
  readBank = 0;
  readAddress = 0;
  writeBank = 0;

  r6003 = 0x00;
  r6004 = 0xff;
  r6005 = 0xff;
  r6006 = 0xff;
  r6007 = 0xff;
  for(auto& r : r7000) r = 0x00;
  mltReq = 0;

  hcounter = 0;
  vcounter = 0;

  GameBoy::system.power();
  GameBoy::apu.stream->setFrequency(clockFrequency() / 5.0 / 2.0);
}

#endif
