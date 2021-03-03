#if defined(CORE_GB)

struct ICD : Platform, GameBoy::SuperGameBoyInterface, Thread {
  Node::System node;

  //icd.cpp
  auto clockFrequency() const -> f64;

  auto load(Node::Peripheral) -> void;
  auto unload() -> void;
  auto save() -> void;

  auto main() -> void;
  auto power(bool reset = false) -> void;

  //interface.cpp
  auto ppuHreset() -> void override;
  auto ppuVreset() -> void override;
  auto ppuWrite(n2 color) -> void override;
  auto joypWrite(n1 p14, n1 p15) -> void override;

  //io.cpp
  auto readIO(n24 address, n8 data) -> n8;
  auto writeIO(n24 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n32 Revision = 0;
  n32 Frequency = 0;

private:
  struct Packet {
    auto operator[](n4 address) -> n8& { return data[address]; }
    n8 data[16];
  };
  Packet packet[64];
  n7 packetSize;

  n2 joypID;
  n1 joypLock;
  n1 pulseLock;
  n1 strobeLock;
  n1 packetLock;
  Packet joypPacket;
  n4 packetOffset;
  n8 bitData;
  n3 bitOffset;

  n8 output[4 * 512];
  n2 readBank;
  n9 readAddress;
  n2 writeBank;

  n8 r6003;      //control port
  n8 r6004;      //joypad 1
  n8 r6005;      //joypad 2
  n8 r6006;      //joypad 3
  n8 r6007;      //joypad 4
  n8 r7000[16];  //JOYP packet data
  n8 mltReq;     //number of active joypads

  n8 hcounter;
  n8 vcounter;
};

#else

struct ICD : Thread {
  Node::Port port;
  Node::Peripheral node;

  auto name() const -> string { return {}; }

  auto load(Node::Peripheral) -> void {}
  auto unload() -> void {}
  auto save() -> void {}

  auto power(bool reset = false) -> void {}

  auto readIO(n24, n8) -> n8 { return 0; }
  auto writeIO(n24, n8) -> void { return; }

  auto serialize(serializer&) -> void {}

  n32 Revision = 0;
  n32 Frequency = 0;
};

#endif

extern ICD icd;
