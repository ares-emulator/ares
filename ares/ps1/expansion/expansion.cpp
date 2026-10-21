#include <ps1/ps1.hpp>

namespace ares::PlayStation {

ExpansionPort expansion;
#include "datel/datel.cpp"
#include "xplorer/xplorer.cpp"

auto ExpansionDevice::readHalf(u32 address) -> u32 { return readByte(address) | readByte(address + 1) << 8; }
auto ExpansionDevice::readWord(u32 address) -> u32 { return readHalf(address) | readHalf(address + 2) << 16; }
auto ExpansionDevice::writeHalf(u32 address, u32 data) -> void { writeByte(address, data); writeByte(address + 1, data >> 8); }
auto ExpansionDevice::writeWord(u32 address, u32 data) -> void { writeHalf(address, data); writeHalf(address + 2, data >> 16); }

auto ExpansionPort::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>("Parallel Port");
  port->setFamily("PlayStation");
  port->setType("Expansion");
  port->setHotSwappable(false);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setDisconnect([&] { device.reset(); });
  port->setSupported({"Datel (128KB)", "Datel (256KB)", "Xplorer", "Xplorer FX (Atmel)", "Xplorer FX (Winbond)"});
}

auto ExpansionPort::unload() -> void {
  device.reset();
  port.reset();
}

auto ExpansionPort::allocate(string name) -> Node::Peripheral {
  if(name == "Datel (128KB)") device = std::make_unique<Datel>(port, Datel128ID);
  if(name == "Datel (256KB)") device = std::make_unique<Datel>(port, DatelID);
  if(name == "Xplorer") device = std::make_unique<Xplorer>(port, XplorerID);
  if(name == "Xplorer FX (Winbond)") device = std::make_unique<Xplorer>(port, XplorerFXID);
  if(name == "Xplorer FX (Atmel)") device = std::make_unique<Xplorer>(port, XplorerFXAtmelID);
  if(!device || !device->node) { device.reset(); return {}; }
  return device->node;
}

auto ExpansionPort::save() -> void { if(device) device->save(); }
auto ExpansionPort::power(bool reset) -> void { if(device) device->power(reset); }
auto ExpansionPort::step(u32 clocks) -> void { if(device) device->step(clocks); }
auto ExpansionPort::serialize(serializer& s) -> void { if(device) device->serialize(s); }
auto ExpansionPort::readByte(u32 address) -> u32 { return device ? device->readByte(address) : 0xff; }
auto ExpansionPort::readHalf(u32 address) -> u32 { return device ? device->readHalf(address) : 0xffff; }
auto ExpansionPort::readWord(u32 address) -> u32 { return device ? device->readWord(address) : 0xffff'ffff; }
auto ExpansionPort::writeByte(u32 address, u32 data) -> void { if(device) device->writeByte(address, data); }
auto ExpansionPort::writeHalf(u32 address, u32 data) -> void { if(device) device->writeHalf(address, data); }
auto ExpansionPort::writeWord(u32 address, u32 data) -> void { if(device) device->writeWord(address, data); }

}
