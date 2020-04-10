#if defined(CORE_GB)

struct ICD : Platform, GameBoy::SuperGameBoyInterface, Thread {
  //icd.cpp
  auto clockFrequency() const -> double;

  auto load(Node::Peripheral) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto power(bool reset = false) -> void;

  //interface.cpp
  auto ppuHreset() -> void override;
  auto ppuVreset() -> void override;
  auto ppuWrite(uint2 color) -> void override;
  auto joypWrite(uint1 p14, uint1 p15) -> void override;

  //io.cpp
  auto readIO(uint24 address, uint8 data) -> uint8;
  auto writeIO(uint24 address, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  uint Revision = 0;
  uint Frequency = 0;

private:
  struct Packet {
    auto operator[](uint4 address) -> uint8& { return data[address]; }
    uint8 data[16];
  };
  Packet packet[64];
  uint7 packetSize;

  uint2 joypID;
  uint1 joypLock;
  uint1 pulseLock;
  uint1 strobeLock;
  uint1 packetLock;
  Packet joypPacket;
  uint4 packetOffset;
  uint8 bitData;
  uint3 bitOffset;

  uint8 output[4 * 512];
  uint2 readBank;
  uint9 readAddress;
  uint2 writeBank;

  uint8 r6003;      //control port
  uint8 r6004;      //joypad 1
  uint8 r6005;      //joypad 2
  uint8 r6006;      //joypad 3
  uint8 r6007;      //joypad 4
  uint8 r7000[16];  //JOYP packet data
  uint8 mltReq;     //number of active joypads

  uint8 hcounter;
  uint8 vcounter;
};

#else

struct ICD : Thread {
  Node::Port port;
  Node::Peripheral node;

  auto name() const -> string { return {}; }

  auto load(Node::Peripheral) -> void {}
  auto unload() -> void {}

  auto connect(Node::Peripheral) -> void {}
  auto disconnect() -> void {}
  auto power(bool reset = false) -> void {}

  auto readIO(uint24, uint8) -> uint8 { return 0; }
  auto writeIO(uint24, uint8) -> void { return; }

  auto serialize(serializer&) -> void {}

  uint Revision = 0;
  uint Frequency = 0;
};

#endif

extern ICD icd;
