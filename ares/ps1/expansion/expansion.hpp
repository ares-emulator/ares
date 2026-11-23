struct ExpansionDevice : Memory::Interface {
  Node::Peripheral node;

  virtual ~ExpansionDevice() = default;
  virtual auto save() -> void = 0;
  virtual auto power(bool reset) -> void = 0;
  virtual auto step(u32 clocks) -> void = 0;
  virtual auto serialize(serializer&) -> void = 0;
  virtual auto type() const -> u32 = 0;
  virtual auto canReadDMA() -> bool { return false; }
  virtual auto canWriteDMA() -> bool { return false; }
  virtual auto readDMA() -> u32 { return 0xffff'ffff; }
  virtual auto writeDMA(u32 data) -> void {}
  auto readByte(u32 address) -> u32 override = 0;
  auto writeByte(u32 address, u32 data) -> void override = 0;
  auto readHalf(u32 address) -> u32 override;
  auto readWord(u32 address) -> u32 override;
  auto writeHalf(u32 address, u32 data) -> void override;
  auto writeWord(u32 address, u32 data) -> void override;
};

struct ExpansionPort : Memory::Interface {
  enum Device : u32 { None, DatelID, XplorerID, XplorerFXID, Datel128ID, XplorerFXAtmelID, KonamiGVID };
  Node::Port port;
  std::unique_ptr<ExpansionDevice> device;

  auto connected() const -> bool { return (bool)device; }
  auto type() const -> u32 { return device ? device->type() : None; }
  auto load(Node::Object parent) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;
  auto save() -> void;
  auto power(bool reset) -> void;
  auto step(u32 clocks) -> void;
  auto serialize(serializer&) -> void;
  auto canReadDMA() -> bool { return device && device->canReadDMA(); }
  auto canWriteDMA() -> bool { return device && device->canWriteDMA(); }
  auto readDMA() -> u32 { return device ? device->readDMA() : 0xffff'ffff; }
  auto writeDMA(u32 data) -> void { if(device) device->writeDMA(data); }
  auto readByte(u32 address) -> u32 override;
  auto readHalf(u32 address) -> u32 override;
  auto readWord(u32 address) -> u32 override;
  auto writeByte(u32 address, u32 data) -> void override;
  auto writeHalf(u32 address, u32 data) -> void override;
  auto writeWord(u32 address, u32 data) -> void override;
};

#include "datel/datel.hpp"
#include "xplorer/xplorer.hpp"
#include "konami-gv/konami-gv.hpp"

extern ExpansionPort expansion;
