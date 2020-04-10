S21FX::S21FX(Node::Port parent) {
  node = parent->append<Node::Peripheral>("21fx");

  Thread::create(10'000'000, {&S21FX::main, this});

  resetVector.byte(0) = bus.read(0xfffc, 0x00);
  resetVector.byte(1) = bus.read(0xfffd, 0x00);

  bus.map({&S21FX::read, this}, {&S21FX::write, this}, "00-3f,80-bf:2184-21ff");
  bus.map({&S21FX::read, this}, {&S21FX::write, this}, "00:fffc-fffd");

  booted = false;

  for(auto& byte : ram) byte = 0xdb;  //stp
  ram[0] = 0x6c;  //jmp ($fffc)
  ram[1] = 0xfc;
  ram[2] = 0xff;

  if(auto fp = platform->open(node, "21fx.rom", File::Read, File::Required)) {
    fp->read(ram, sizeof(ram));
  }

  //dynamic libraries can only be loaded via filename ...
  //there's not really much choice but to copy the library to a temporary directory here
  if(auto fp = platform->open(node, "21fx.so", File::Read, File::Required)) {
    if(auto buffer = file::open({Path::temporary(), "21fx.so"}, file::mode::write)) {
      for(uint index : range(fp->size())) buffer.write(fp->read());
    }
  }

  string filename{Path::temporary(), "21fx.so"};
  if(link.openAbsolute(filename)) {
    linkInit = link.sym("fx_init");
    linkMain = link.sym("fx_main");
  }
}

S21FX::~S21FX() {
  scheduler.remove(*this);
  bus.unmap("00-3f,80-bf:2184-21ff");
  bus.unmap("00:fffc-fffd");

  //note: this is an awful hack ...
  //since the bus maps are lambdas, we can't safely restore the original reset vector handler
  //as such, we install a basic read-only lambda that simply returns the known reset vector
  //the downside is that if 00:fffc-fffd were anything but ROM; it will now only act as ROM
  //given that this is the only device that hooks the reset vector like this,
  //it's not worth the added complexity to support some form of reversible bus mapping hooks
  uint vector = resetVector;
  bus.map([vector](uint24 addr, uint8) -> uint8 {
    return vector >> addr * 8;
  }, [](uint24, uint8) -> void {
  }, "00:fffc-fffd", 2);

  if(link.open()) link.close();
  linkInit.reset();
  linkMain.reset();
}

auto S21FX::step(uint clocks) -> void {
  Thread::step(clocks);
  synchronize(cpu);
}

auto S21FX::main() -> void {
  if(linkInit) linkInit(
    {&S21FX::quit, this},
    {&S21FX::usleep, this},
    {&S21FX::readable, this},
    {&S21FX::writable, this},
    {&S21FX::read, this},
    {&S21FX::write, this}
  );
  if(linkMain) linkMain({});
  while(true) scheduler.synchronize(), step(10'000'000);
}

auto S21FX::read(uint24 address, uint8 data) -> uint8 {
  address &= 0x40ffff;

  if(address == 0xfffc) return booted ? resetVector >> 0 : (0x84);
  if(address == 0xfffd) return booted ? resetVector >> 8 : (booted = true, 0x21);

  if(address >= 0x2184 && address <= 0x21fd) return ram[address - 0x2184];

  if(address == 0x21fe) return !link.open() ? 0 : (
    (linkBuffer.size() >    0) << 7  //1 = readable
  | (snesBuffer.size() < 1024) << 6  //1 = writable
  | (link.open())              << 5  //1 = connected
  );

  if(address == 0x21ff) {
    if(linkBuffer.size() > 0) {
      return linkBuffer.takeLeft();
    }
  }

  return data;
}

auto S21FX::write(uint24 address, uint8 data) -> void {
  address &= 0x40ffff;

  if(address == 0x21ff) {
    if(snesBuffer.size() < 1024) {
      snesBuffer.append(data);
    }
  }
}

auto S21FX::quit() -> bool {
  step(1);
  return false;
}

auto S21FX::usleep(uint microseconds) -> void {
  step(10 * microseconds);
}

auto S21FX::readable() -> bool {
  step(1);
  return snesBuffer.size() > 0;
}

auto S21FX::writable() -> bool {
  step(1);
  return linkBuffer.size() < 1024;
}

//SNES -> Link
auto S21FX::read() -> uint8 {
  step(1);
  if(snesBuffer.size() > 0) {
    return snesBuffer.takeLeft();
  }
  return 0x00;
}

//Link -> SNES
auto S21FX::write(uint8 data) -> void {
  step(1);
  if(linkBuffer.size() < 1024) {
    linkBuffer.append(data);
  }
}
