#include <msx/msx.hpp>

namespace ares::MSX {

Cartridge cartridge{"Cartridge Slot"};
Cartridge expansion{"Expansion Slot"};
#include "serialization.cpp"

Cartridge::Cartridge(string_view name) : portName(name) {
}

auto Cartridge::load(Node::Object parent, Node::Object from) -> void {
  port = Node::append<Node::Port>(parent, from, portName);
  port->setFamily(interface->name());
  port->setType("Cartridge");
  port->setAllocate([&] { return Node::Peripheral::create(interface->name()); });
  port->setAttach([&](auto node) { connect(node); });
  port->setDetach([&](auto node) { disconnect(); });
  port->scan(from);
}

auto Cartridge::unload() -> void {
  disconnect();
  port = {};
}

auto Cartridge::connect(Node::Peripheral with) -> void {
  node = Node::append<Node::Peripheral>(port, with, interface->name());
  node->setManifest([&] { return information.manifest; });

  information = {};

  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].text();
  information.region = document["game/region"].text();

  if(auto memory = document["game/board/memory(type=ROM,content=Program)"]) {
    rom.allocate(memory["size"].natural());
    if(auto fp = platform->open(node, "program.rom", File::Read, File::Required)) {
      rom.load(fp);
    }
  }

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    ram.allocate(memory["size"].natural());
    if(auto fp = platform->open(node, "save.ram", File::Read)) {
      ram.load(fp);
    }
  }

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  save();
  rom.reset();
  ram.reset();
  node = {};
}

auto Cartridge::save() -> void {
  if(!node) return;
  auto document = BML::unserialize(information.manifest);

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    if(auto fp = platform->open(node, "save.ram", File::Write)) {
      ram.save(fp);
    }
  }
}

auto Cartridge::power() -> void {
}

uint2 RB;  //Cross Blaim
uint8 B8K[4]={0,1,0,0};  //Konami 8K (non-SCC)

auto Cartridge::read(uint16 address) -> uint8 {
  if(!rom) return 0xff;

/*
if(RB==0||RB==1){
  if(address>>14==0)return rom.read((uint14)address+16_KiB);
  if(address>>14==1)return rom.read((uint14)address);
  if(address>>14==2)return rom.read((uint14)address+16_KiB);
  if(address>>14==3)return rom.read((uint14)address+16_KiB);
}
if(RB==2){
  if(address>>14==0)return 0xff;
  if(address>>14==1)return rom.read((uint14)address);
  if(address>>14==2)return rom.read((uint14)address+32_KiB);
  if(address>>14==3)return 0xff;
}
if(RB==3){
  if(address>>14==0)return 0xff;
  if(address>>14==1)return rom.read((uint14)address);
  if(address>>14==2)return rom.read((uint14)address+48_KiB);
  if(address>>14==3)return 0xff;
}
*/

if(address>>13==2)return rom.read(B8K[0]<<13|(uint13)address);
if(address>>13==3)return rom.read(B8K[1]<<13|(uint13)address);
if(address>>13==4)return rom.read(B8K[2]<<13|(uint13)address);
if(address>>13==5)return rom.read(B8K[3]<<13|(uint13)address);

  return rom.read(address);
}

auto Cartridge::write(uint16 address, uint8 data) -> void {
  RB = data;
if(address>>13==2)B8K[0]=0;
if(address>>13==3)B8K[1]=data;
if(address>>13==4)B8K[2]=data;
if(address>>13==5)B8K[3]=data;
}

}
