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
      u32 offset = address - 0x0800'0000;
      if(self.ram.size > 32_KiB) {
        u32 bank = offset >> 18;
        if(bank >= self.ram.size >> 15) return false;
        if((offset & 0x3ffff) >= 0x8000) return false;
        piView = {self.ram.data + (bank << 15), 0x8000};
        piViewOffset = offset & 0x7fff;
      } else {
        piView = {self.ram.data, self.ram.size};
        piViewOffset = offset & self.ram.maskByte;
      }
      piViewWritable = true;
      return true;
    }
  } ramDevice{*this};

  struct Flash : Memory::Writable, PIDevice {
    struct Model {
      const char* name;
      u16 manufacturerId;
      u16 deviceId;
      bool wordIndexed;
      u32 sectorEraseClocks;
      u32 chipEraseClocks;
      u32 programClocks;
    };
    static const Model models[];

    auto piAddress(u32 address, PIDeviceTiming timing) -> bool override;
    auto piReadHalf(PIDeviceTiming timing) -> maybe<u16> override;
    auto piWriteHalf(u16 data, PIDeviceTiming timing) -> void override;

    auto setModel(string name) -> void;
    auto power(bool reset) -> void;
    auto finish() -> void;
    auto serialize(serializer& s) -> void;

    auto macronix() const -> bool { return model->manufacturerId == 0x00c2; }
    auto matsushita() const -> bool { return model->manufacturerId == 0x0032; }

    enum class Mode : u32 { ReadArray, Status, SiliconID, LoadBytePage };
    enum class EraseSetup : u32 { None, Chip, Sector };
    enum class Busy : u32 { None, Erase, Program };

    struct Status {
      n8 data = 0;
      auto wsmReady()     { return data.bit(7); }
      auto eraseOk()      { return data.bit(3); }
      auto programOk()    { return data.bit(2); }
      auto eraseBusy()    { return data.bit(1); }
      auto programBusy()  { return data.bit(0); }
      auto ok()           { return data.bit(2, 3); }
      auto busy()         { return data.bit(0, 1); }
      auto serialize(serializer& s) -> void { s(data); }
    };

    const Model* model = &models[3];
    Mode mode = Mode::ReadArray;
    Status status;
    EraseSetup eraseSetup = EraseSetup::None;
    n3  eraseSector = 0;
    Busy busy = Busy::None;
    u8  pageBuffer[128];
    u32 piOffset = 0;
    n1  cirHighValid;
    u16 cirHigh = 0;
    n1  openBus;
    n1  statusStale;
    u16 statusStaleValue = 0;
    u8  pendingModeCmd = 0;
    u8  pendingModeCount = 0;
    u16 burstIndex = 0;

  private:
    auto command(u32 data) -> void;
    auto wrapMask() const -> u32 { return model->wordIndexed ? 0x3fff : 0x7fff; }
    auto arrayOffset(u32 flashOffset) const -> u32;
    auto siliconHalf(u32 halfIndex) const -> u16;
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
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;
    auto flash(u32 command) -> void;

    struct Memory {
      Node::Debugger::Memory rom;
      Node::Debugger::Memory ram;
      Node::Debugger::Memory eeprom;
      Node::Debugger::Memory flash;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Notification flash;
    } tracer;
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
