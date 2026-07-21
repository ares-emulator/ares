struct Cartridge {
  Node::Peripheral node;
  VFS::Pak pak;
  Memory::Readable16 rom;
  Memory::Writable16 ram;

  struct RomDevice : PIDeviceMemory {
    Cartridge& self;
    RomDevice(Cartridge& self) : self(self) {}
    auto piAddress(u32 address, PIDeviceTiming timing) -> bool override {
      if(!timing.fasterThan(min)) return false;
      if(address < 0x1000'0000 || address >= 0x1000'0000 + self.rom.size) return false;
      piView = {self.rom.data, self.rom.size};
      piViewOffset = address - 0x1000'0000;
      piViewWritable = false;
      return true;
    }
  } romDevice{*this};

  struct RamDevice : PIDeviceMemory {
    Cartridge& self;
    RamDevice(Cartridge& self) : self(self) {}
    auto piAddress(u32 address, PIDeviceTiming timing) -> bool override {
      if(!timing.fasterThan(min)) return false;
      if(address < 0x0800'0000 || address > 0x0fff'ffff) return false;
      piView = {self.ram.data, self.ram.size};
      piViewOffset = (address - 0x0800'0000) & self.ram.maskByte;
      piViewWritable = true;
      return true;
    }
  } ramDevice{*this};

  struct Flash : Memory::Writable, PIDevice {
    auto piAddress(u32 address, PIDeviceTiming timing) -> bool override;
    auto piReadHalf(PIDeviceTiming timing) -> maybe<u16> override;
    auto piWriteHalf(u16 data, PIDeviceTiming timing) -> void override;

    //flash.cpp
    auto readHalf(u32 address) -> u64;
    auto writeHalf(u32 address, u64 data) -> void;
    auto writeWord(u32 address, u64 data) -> void;
    auto serialize(serializer& s) -> void;

    enum class Mode : u32 { Idle, Erase, Write, Read, Status };
    Mode mode = Mode::Idle;
    u64  status = 0;
    u32  source = 0;
    u32  offset = 0;
    u32  piAddr = 0;
    u16  writeLatch = 0;
    n1   writeLatchValid;
  } flash;

  struct ISViewer : PIDevice {
    Memory::Writable ram;
    Node::Debugger::Tracer::Notification tracer;
    u32 piAddr = 0;

    auto enabled() -> bool { return ram.size; }

    auto piAddress(u32 address, PIDeviceTiming timing) -> bool override;
    auto piReadHalf(PIDeviceTiming timing) -> maybe<u16> override;
    auto piWriteHalf(u16 data, PIDeviceTiming timing) -> void override;

    //isviewer.cpp
    auto messageChar(char c) -> void;
    auto readHalf(u32 address) -> u16;
    auto writeHalf(u32 address, u16 data) -> void;
  } isviewer;

  struct RTC {
    Cartridge& self;
    RTC(Cartridge &self) : self(self) {}

    Memory::Writable ram;
    n1 present;
    n8 status;
    n3 writeLock;

    // rtc.cpp
    auto power(bool reset) -> void;
    auto run(bool run) -> void;
    auto running() -> bool;
    auto load() -> void;
    auto save() -> void;
    auto tick(int nsec=1) -> void;
    auto advance(int nsec) -> void;
    auto serialize(serializer& s) -> void;
    auto read(u2 block, n8 *data) -> void;
    auto write(u2 block, n8 *data) -> void;
  } rtc{*this};

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory rom;
      Node::Debugger::Memory ram;
      Node::Debugger::Memory eeprom;
      Node::Debugger::Memory flash;
    } memory;
  } debugger;

  Memory::Writable16 eeprom;
  n1 eepromBusy;

  auto title() const -> string { return information.title; }
  auto region() const -> string { return information.region; }
  auto cic() const -> string { return information.cic; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;
  auto save() -> void;
  auto power(bool reset) -> void;

  //joybus.cpp
  auto joybusComm(n8 send, n8 recv, n8 input[], n8 output[]) -> n2;
  auto eepromFinish() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct Information {
    string title;
    string region;
    string cic;
  } information;
};

#include "slot.hpp"
extern Cartridge& cartridge;
