struct Cartridge {
  Node::Peripheral node;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }
  auto region() const -> string { return information.region; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto power(bool reset) -> void;
  auto save() -> void;

  auto lookupMemory(Markup::Node) -> Markup::Node;
  auto lookupOscillator() -> Markup::Node;

  auto serialize(serializer&) -> void;

  ReadableMemory rom;
  WritableMemory ram;

  struct Information {
    string manifest;
    Markup::Node document;
    string name;
    string region;
    string board;
  } information;

  struct Has {
    boolean ICD;
    boolean MCC;
    boolean DIP;
    boolean Competition;
    boolean SA1;
    boolean SuperFX;
    boolean ARMDSP;
    boolean HitachiDSP;
    boolean NECDSP;
    boolean EpsonRTC;
    boolean SharpRTC;
    boolean SPC7110;
    boolean SDD1;
    boolean OBC1;
    boolean MSU1;

    boolean GameBoySlot;
    boolean BSMemorySlot;
    boolean SufamiTurboSlotA;
    boolean SufamiTurboSlotB;
  } has;

private:
  Markup::Node board;

  //load.cpp
  auto loadBoard(string) -> Markup::Node;
  auto loadCartridge(Markup::Node) -> void;
  auto loadMemory(AbstractMemory&, Markup::Node, bool required) -> void;
  template<typename T> auto loadMap(Markup::Node, T&) -> uint;
  auto loadMap(Markup::Node, const function<uint8 (uint24, uint8)>&, const function<void (uint24, uint8)>&) -> uint;

  auto loadROM(Markup::Node) -> void;
  auto loadRAM(Markup::Node) -> void;
  auto loadICD(Markup::Node) -> void;
  auto loadMCC(Markup::Node) -> void;
  auto loadBSMemory(Markup::Node) -> void;
  auto loadSufamiTurboA(Markup::Node) -> void;
  auto loadSufamiTurboB(Markup::Node) -> void;
  auto loadDIP(Markup::Node) -> void;
  auto loadCompetition(Markup::Node) -> void;
  auto loadSA1(Markup::Node) -> void;
  auto loadSuperFX(Markup::Node) -> void;
  auto loadARMDSP(Markup::Node) -> void;
  auto loadHitachiDSP(Markup::Node, uint roms) -> void;
  auto loaduPD7725(Markup::Node) -> void;
  auto loaduPD96050(Markup::Node) -> void;
  auto loadEpsonRTC(Markup::Node) -> void;
  auto loadSharpRTC(Markup::Node) -> void;
  auto loadSPC7110(Markup::Node) -> void;
  auto loadSDD1(Markup::Node) -> void;
  auto loadOBC1(Markup::Node) -> void;
  auto loadMSU1() -> void;

  //save.cpp
  auto saveCartridge(Markup::Node) -> void;
  auto saveMemory(AbstractMemory&, Markup::Node) -> void;

  auto saveRAM(Markup::Node) -> void;
  auto saveMCC(Markup::Node) -> void;
  auto saveSA1(Markup::Node) -> void;
  auto saveSuperFX(Markup::Node) -> void;
  auto saveARMDSP(Markup::Node) -> void;
  auto saveHitachiDSP(Markup::Node) -> void;
  auto saveuPD7725(Markup::Node) -> void;
  auto saveuPD96050(Markup::Node) -> void;
  auto saveEpsonRTC(Markup::Node) -> void;
  auto saveSharpRTC(Markup::Node) -> void;
  auto saveSPC7110(Markup::Node) -> void;
  auto saveOBC1(Markup::Node) -> void;

  friend class Interface;
  friend class ICD;
};

#include "slot.hpp"
extern Cartridge& cartridge;
