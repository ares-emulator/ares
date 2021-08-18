namespace Board {

#include "linear.cpp"
#include "banked.cpp"
#include "svp.cpp"
#include "j-cart.cpp"
#include "game-genie.cpp"
#include "mega-32x.cpp"
#include "debugger.cpp"

auto Interface::main() -> void {
  step(cartridge->frequency());
}

auto Interface::step(u32 clocks) -> void {
  cartridge->step(clocks);
}

auto Interface::load(Memory::Readable<n16>& rom, string name) -> bool {
  rom.reset();
  if(auto fp = pak->read(name)) {
    rom.allocate(fp->size() >> 1);
    for(auto address : range(rom.size())) rom.program(address, fp->readm(2L));
    return true;
  }
  return false;
}

auto Interface::load(Memory::Writable<n16>& wram, Memory::Writable<n8>& uram, Memory::Writable<n8>& lram, string name) -> bool {
  wram.reset();
  uram.reset();
  lram.reset();
  if(auto fp = pak->read(name)) {
    auto mode = fp->attribute("mode");
    if(mode == "word") {
      wram.allocate(fp->size() >> 1);
      for(auto address : range(wram.size())) wram.write(address, fp->readm(2L));
      return true;
    } else if(mode == "upper") {
      uram.allocate(fp->size());
      for(auto address : range(uram.size())) uram.write(address, fp->readm(1L));
      return true;
    } else if(mode == "lower") {
      lram.allocate(fp->size());
      for(auto address : range(lram.size())) lram.write(address, fp->readm(1L));
      return true;
    }
  }
  return false;
}

auto Interface::load(M24C& m24c, string name) -> bool {
  m24c.reset();
  if(auto fp = pak->read(name)) {
    auto mode = fp->attribute("mode");
    m24c.reset();
    if(mode == "X24C01" ) m24c.load(M24C::Type::X24C01 );
    if(mode == "M24C01" ) m24c.load(M24C::Type::M24C01 );
    if(mode == "M24C02" ) m24c.load(M24C::Type::M24C02 );
    if(mode == "M24C04" ) m24c.load(M24C::Type::M24C04 );
    if(mode == "M24C08" ) m24c.load(M24C::Type::M24C08 );
    if(mode == "M24C16" ) m24c.load(M24C::Type::M24C16 );
    if(mode == "M24C32" ) m24c.load(M24C::Type::M24C32 );
    if(mode == "M24C64" ) m24c.load(M24C::Type::M24C64 );
    if(mode == "M24C65" ) m24c.load(M24C::Type::M24C65 );
    if(mode == "M24C128") m24c.load(M24C::Type::M24C128);
    if(mode == "M24C256") m24c.load(M24C::Type::M24C256);
    if(mode == "M24C512") m24c.load(M24C::Type::M24C512);
    if(m24c) {
      fp->read({m24c.memory, m24c.size()});
      return true;
    }
  }
  return false;
}

auto Interface::save(Memory::Writable<n16>& wram, Memory::Writable<n8>& uram, Memory::Writable<n8>& lram, string name) -> bool {
  if(auto fp = pak->write(name)) {
    auto mode = fp->attribute("mode");
    if(mode == "word") {
      for(auto address : range(wram.size())) fp->writem(wram[address], 2L);
      return true;
    } else if(mode == "upper") {
      for(auto address : range(uram.size())) fp->writem(uram[address], 1L);
      return true;
    } else if(mode == "lower") {
      for(auto address : range(lram.size())) fp->writem(lram[address], 1L);
      return true;
    }
  }
  return false;
}

auto Interface::save(M24C& m24c, string name) -> bool {
  if(auto fp = pak->write(name)) {
    if(m24c) {
      fp->write({m24c.memory, m24c.size()});
      return true;
    }
  }
  return false;
}

}
