BSMemoryCartridge& bsmemory = bsmemorySlot.cartridge;
#include "slot.cpp"
#include "serialization.cpp"

auto BSMemoryCartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>("BS Memory Cartridge");
}

auto BSMemoryCartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;
  information = {};
  information.title = pak->attribute("title");

  if(auto fp = pak->read("program.rom")) {
    ROM = 1;
    memory.allocate(fp->size());
    fp->read({memory.data(), memory.size()});
  }

  if(auto fp = pak->read("program.flash")) {
    ROM = 0;
    memory.allocate(max(fp->size(), 0x100000));
    fp->read({memory.data(), memory.size()});
  }

  //some BS Memory cassetes use ROM chips rather than Flash chips ...
  //do not emulate a flash chip interface for ROM cassettes.
  if(ROM) return;

  if(size() != 0x100000 && size() != 0x200000 && size() != 0x400000) {
    memory.reset();
    return;
  }

  chip.vendor = 0x00'b0;  //Sharp
  if(size() == 0x100000) chip.device = 0x66'a8;  //LH28F800SU
  if(size() == 0x200000) chip.device = 0x66'88;  //LH28F016SU
  if(size() == 0x400000) chip.device = 0x66'88;  //LH28F032SU (same device ID as LH28F016SU per datasheet)
  chip.serial = 0x00'01'23'45'67'89ull;  //serial# should be unique for every cartridge ...

  //page buffer values decay to random noise upon losing power to the flash chip
  //the randomness is high entropy (at least compared to SNES SRAM/DRAM chips)
  for(auto& byte : page.buffer[0]) byte = random();
  for(auto& byte : page.buffer[1]) byte = random();

  for(auto& block : blocks) {
    block.erased = 1;
    block.locked = 1;
  }

  if(auto fp = pak->read("flash.bml")) {
    auto document = BML::unserialize(fp->reads());
    if(auto node = document["flash/vendor"]) {
      chip.vendor = node.natural();
    }
    if(auto node = document["flash/device"]) {
      chip.device = node.natural();
    }
    if(auto node = document["flash/serial"]) {
      chip.serial = node.natural();
    }
    for(u32 id : range(block.count())) {
      if(auto node = document[{"flash/block(id=", id, ")"}]) {
        if(auto erased = node["erased"]) {
          block(id).erased = erased.natural();
        }
        if(auto locked = node["locked"]) {
          block(id).locked = locked.boolean();
        }
      }
    }
  }

  power();
}

auto BSMemoryCartridge::disconnect() -> void {
  if(!node) return;

  if(!ROM) {
    if(auto fp = pak->write("flash.bml")) {
      string manifest;
      manifest.append("flash\n");
      manifest.append("  vendor: 0x", hex(chip.vendor,  4L), "\n");
      manifest.append("  device: 0x", hex(chip.device,  4L), "\n");
      manifest.append("  serial: 0x", hex(chip.serial, 12L), "\n");
      for(n6 id : range(block.count())) {
        manifest.append("  block\n");
        manifest.append("    id: ", id, "\n");
        manifest.append("    erased: ",  (u32)block(id).erased, "\n");
        manifest.append("    locked: ", (bool)block(id).locked, "\n");
      }
      fp->writes(manifest);
    }
    std::erase(cpu.coprocessors, this);
    Thread::destroy();
  }

  memory.reset();
  pak.reset();
  node.reset();
}

BSMemoryCartridge::BSMemoryCartridge() {
  page.self = this;
  u32 blockID = 0;
  for(auto& block : blocks) block.self = this, block.id = blockID++;
  block.self = this;
}

auto BSMemoryCartridge::main() -> void {
  for(n6 id : range(block.count())) {
    if(block(id).erasing) return block(id).erase();
    block(id).status.ready = 1;
  }

  compatible.status.ready = 1;
  global.status.ready = 1;
  step(10'000);  //10 milliseconds
}

auto BSMemoryCartridge::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto BSMemoryCartridge::power() -> void {
  if(ROM) return;

  Thread::create(1'000'000, {&BSMemoryCartridge::main, this});  //microseconds
  cpu.coprocessors.push_back(this);

  for(auto& block : blocks) {
    block.erasing = 0;
    block.status = {};
  }
  compatible.status = {};
  global.status = {};
  mode = Mode::Flash;
  readyBusyMode = ReadyBusyMode::Disable;
  queue.flush();
}

auto BSMemoryCartridge::save() -> void {
  if(!node) return;

  if(auto fp = pak->write("program.flash")) {
    fp->write({memory.data(), memory.size()});
  }
}

auto BSMemoryCartridge::read(n24 address, n8 data) -> n8 {
  if(!memory) return data;
  if(ROM) return memory.read(bus.mirror(address, size()));

  if(mode == Mode::Chip) {
    if(address == 0) return chip.vendor.byte(0);  //only appears once
    if(address == 1) return chip.device.byte(0);  //only appears once
    if((n3)address == 2) return 0x63;  //unknown constant: repeats every eight bytes
    return 0x20;  //unknown constant: fills in all remaining bytes
  }

  if(mode == Mode::Page) {
    return page.read(address);
  }

  if(mode == Mode::CompatibleStatus) {
    return compatible.status();
  }

  if(mode == Mode::ExtendedStatus) {
    if((n16)address == 0x0002) return block(address >> block.bits()).status();
    if((n16)address == 0x0004) return global.status();
    return 0x00;  //reserved: always zero
  }

  return block(address >> block.bits()).read(address);  //Mode::Flash
}

auto BSMemoryCartridge::write(n24 address, n8 data) -> void {
  if(!memory || ROM) return;
  queue.push(address, data);

  //write page to flash
  if(queue.data(0) == 0x0c) {
  if(queue.size() < 3) return;
    n16 count;  //1 - 65536
    count.byte(0) = queue.data(!queue.address(1).bit(0) ? 1 : 2);
    count.byte(1) = queue.data(!queue.address(1).bit(0) ? 2 : 1);
    n24 address = queue.address(2);
    do {
      block(address >> block.bits()).write(address, page.read(address));
      address++;
    } while(count--);
    page.swap();
    mode = Mode::CompatibleStatus;
    return queue.flush();
  }

  //write byte
  if(queue.data(0) == 0x10) {
  if(queue.size() < 2) return;
    block(queue.address(1) >> block.bits()).write(queue.address(1), queue.data(1));
    mode = Mode::CompatibleStatus;
    return queue.flush();
  }

  //erase block
  if(queue.data(0) == 0x20) {
  if(queue.size() < 2) return;
  if(queue.data(1) != 0xd0) return failed(), queue.flush();
    block(queue.address(1) >> block.bits()).erase();
    mode = Mode::CompatibleStatus;
    return queue.flush();
  }

  //LH28F800SUT-ZI specific? (undocumented / unavailable? for the LH28F800SU)
  //write signature, identifier, serial# into current page buffer, then swap page buffers
  if(queue.data(0) == 0x38) {
  if(queue.size() < 2) return;
  if(queue.data(1) != 0xd0) return failed(), queue.flush();
    page.write(0x00, 0x4d);  //'M' (memory)
    page.write(0x02, 0x50);  //'P' (pack)
    page.write(0x04, 0x04);  //unknown constant (maybe block count? eg 1<<4 = 16 blocks)
    page.write(0x06, 0x10 | (n4)log2(size() >> 10));  //d0-d3 = size; d4-d7 = type (1)
    page.write(0x08, chip.serial >> 40);  //serial# (big endian; BCD format)
    page.write(0x0a, chip.serial >> 32);  //smallest observed value:
    page.write(0x0c, chip.serial >> 24);  //  0x00'00'10'62'62'39
    page.write(0x0e, chip.serial >> 16);  //largest observed value:
    page.write(0x10, chip.serial >>  8);  //  0x00'91'90'70'31'03
    page.write(0x12, chip.serial >>  0);  //most values are: 0x00'0x'xx'xx'xx'xx
    page.swap();
    return queue.flush();
  }

  //write byte
  if(queue.data(0) == 0x40) {
  if(queue.size() < 2) return;
    block(queue.address(1) >> block.bits()).write(queue.address(1), queue.data(1));
    mode = Mode::CompatibleStatus;
    return queue.flush();
  }

  //clear status register
  if(queue.data(0) == 0x50) {
    for(n6 id : range(block.count())) {
      block(id).status.vppLow = 0;
      block(id).status.failed = 0;
    }
    compatible.status.vppLow = 0;
    compatible.status.writeFailed = 0;
    compatible.status.eraseFailed = 0;
    global.status.failed = 0;
    return queue.flush();
  }

  //read compatible status register
  if(queue.data(0) == 0x70) {
    mode = Mode::CompatibleStatus;
    return queue.flush();
  }

  //read extended status registers
  if(queue.data(0) == 0x71) {
    mode = Mode::ExtendedStatus;
    return queue.flush();
  }

  //page buffer swap
  if(queue.data(0) == 0x72) {
    page.swap();
    return queue.flush();
  }

  //single load to page buffer
  if(queue.data(0) == 0x74) {
  if(queue.size() < 2) return;
    page.write(queue.address(1), queue.data(1));
    return queue.flush();
  }

  //read page buffer
  if(queue.data(0) == 0x75) {
    mode = Mode::Page;
    return queue.flush();
  }

  //lock block
  if(queue.data(0) == 0x77) {
  if(queue.size() < 2) return;
  if(queue.data(1) != 0xd0) return failed(), queue.flush();
    block(queue.address(1) >> block.bits()).lock();
    return queue.flush();
  }

  //abort
  //(unsupported)
  if(queue.data(0) == 0x80) {
    global.status.sleeping = 1;  //abort seems to put the chip into sleep mode
    return queue.flush();
  }

  //read chip identifiers
  if(queue.data(0) == 0x90) {
    mode = Mode::Chip;
    return queue.flush();
  }

  //update ry/by mode
  //(unsupported)
  if(queue.data(0) == 0x96) {
  if(queue.size() < 2) return;
    if(queue.data(1) == 0x01) readyBusyMode = ReadyBusyMode::EnableToLevelMode;
    if(queue.data(1) == 0x02) readyBusyMode = ReadyBusyMode::PulseOnWrite;
    if(queue.data(1) == 0x03) readyBusyMode = ReadyBusyMode::PulseOnErase;
    if(queue.data(1) == 0x04) readyBusyMode = ReadyBusyMode::Disable;
    return queue.flush();
  }

  //upload lock status bits
  if(queue.data(0) == 0x97) {
  if(queue.size() < 2) return;
  if(queue.data(1) != 0xd0) return failed(), queue.flush();
    for(n6 id : range(block.count())) block(id).update();
    return queue.flush();
  }

  //upload device information (number of erase cycles per block)
  if(queue.data(0) == 0x99) {
  if(queue.size() < 2) return;
  if(queue.data(1) != 0xd0) return failed(), queue.flush();
    page.write(0x06, 0x06);  //unknown constant
    page.write(0x07, 0x00);  //unknown constant
    for(n6 id : range(block.count())) {
      n8 address;
      address += id.bit(0,1) * 0x08;  //verified for LH28F800SUT-ZI
      address += id.bit(2,3) * 0x40;  //verified for LH28F800SUT-ZI
      address += id.bit(4)   * 0x20;  //guessed for LH28F016SU
      address += id.bit(5)   * 0x04;  //guessed for LH28F032SU; will overwrite unknown constants
      n32 erased = 1 << 31 | block(id).erased;  //unknown if d31 is set when erased == 0
      for(n2 byte : range(4)) {
        page.write(address + byte, erased >> byte * 8);  //little endian
      }
    }
    page.swap();
    return queue.flush();
  }

  //erase all blocks
  if(queue.data(0) == 0xa7) {
  if(queue.size() < 2) return;
  if(queue.data(1) != 0xd0) return failed(), queue.flush();
    for(n6 id : range(block.count())) block(id).erase();
    mode = Mode::CompatibleStatus;
    return queue.flush();
  }

  //erase suspend/resume
  //(unsupported)
  if(queue.data(0) == 0xb0) {
  if(queue.size() < 2) return;
  if(queue.data(1) != 0xd0) return failed(), queue.flush();
    mode = Mode::CompatibleStatus;
    return queue.flush();
  }

  //sequential load to page buffer
  if(queue.data(0) == 0xe0) {
  if(queue.size() < 4) return;  //command length = 3 + count
    n16 count;  //1 - 65536
    count.byte(0) = queue.data(1);  //endian order not affected by queue.address(1).field(0)
    count.byte(1) = queue.data(2);
    page.write(queue.address(3), queue.data(3));
    if(count--) {
      queue.history[1].data = count.byte(0);
      queue.history[2].data = count.byte(1);
      return queue.pop();  //hack to avoid needing a 65539-entry queue
    } else {
      return queue.flush();
    }
  }

  //sleep
  //(unsupported)
  if(queue.data(0) == 0xf0) {
    //it is currently unknown how to exit sleep mode; other than via chip reset
    global.status.sleeping = 1;
    return queue.flush();
  }

  //write word
  if(queue.data(0) == 0xfb) {
  if(queue.size() < 3) return;
    n16 value;
    value.byte(0) = queue.data(!queue.address(1).bit(0) ? 1 : 2);
    value.byte(1) = queue.data(!queue.address(1).bit(0) ? 2 : 1);
    //writes are always word-aligned: a0 toggles, rather than increments
    block(queue.address(2) >> block.bits()).write(queue.address(2) ^ 0, value.byte(0));
    block(queue.address(2) >> block.bits()).write(queue.address(2) ^ 1, value.byte(1));
    mode = Mode::CompatibleStatus;
    return queue.flush();
  }

  //read flash memory
  if(queue.data(0) == 0xff) {
    mode = Mode::Flash;
    return queue.flush();
  }

  //unknown command
  return queue.flush();
}

//

auto BSMemoryCartridge::failed() -> void {
  compatible.status.writeFailed = 1;  //datasheet specifies these are for write/erase failures
  compatible.status.eraseFailed = 1;  //yet all errors seem to set both of these bits ...
  global.status.failed = 1;
}

//

auto BSMemoryCartridge::Page::swap() -> void {
  self->global.status.page ^= 1;
}

auto BSMemoryCartridge::Page::read(n8 address) -> n8 {
  return buffer[self->global.status.page][address];
}

auto BSMemoryCartridge::Page::write(n8 address, n8 data) -> void {
  buffer[self->global.status.page][address] = data;
}

//

inline auto BSMemoryCartridge::BlockInformation::bits() const -> u32 { return 16; }
inline auto BSMemoryCartridge::BlockInformation::bytes() const -> u32 { return 1 << bits(); }
inline auto BSMemoryCartridge::BlockInformation::count() const -> u32 { return self->size() >> bits(); }

//

auto BSMemoryCartridge::Block::read(n24 address) -> n8 {
  address &= bytes() - 1;
  return self->memory.read(id << bits() | address);
}

auto BSMemoryCartridge::Block::write(n24 address, n8 data) -> void {
  if(!self->writable() && status.locked) {
    status.failed = 1;
    return self->failed();
  }

  //writes to flash can only clear bits
  address &= bytes() - 1;
  data &= self->memory.read(id << bits() | address);
  self->memory.write(id << bits() | address, data);
}

auto BSMemoryCartridge::Block::erase() -> void {
  if(cpu.active()) {
    //erase command runs even if the block is not currently writable
    erasing = 1;
    status.ready = 0;
    self->compatible.status.ready = 0;
    self->global.status.ready = 0;
    return;
  }

  self->step(300'000);  //300 milliseconds are required to erase one block
  erasing = 0;

  if(!self->writable() && status.locked) {
    //does not set any failure bits when unsuccessful ...
    return;
  }

  for(n24 address : range(bytes())) {
    self->memory.write(id << bits() | address, 0xff);
  }

  erased++;
  locked = 0;
  status.locked = 0;
}

auto BSMemoryCartridge::Block::lock() -> void {
  if(!self->writable()) {
    //produces a failure result even if the page was already locked
    status.failed = 1;
    return self->failed();
  }

  locked = 1;
  status.locked = 1;
}

//at reset, the locked status bit is set
//this command refreshes the true locked status bit from the device
auto BSMemoryCartridge::Block::update() -> void {
  status.locked = locked;
}

//

auto BSMemoryCartridge::Blocks::operator()(n6 id) -> Block& {
  return self->blocks[id & count() - 1];
}

//

auto BSMemoryCartridge::Block::Status::operator()() -> n8 {
  return (  //d0-d1 are reserved; always return zero
    vppLow    << 2
  | queueFull << 3
  | aborted   << 4
  | failed    << 5
  |!locked    << 6  //note: technically the unlocked flag; so locked is inverted here
  | ready     << 7
  );
}

//

auto BSMemoryCartridge::Compatible::Status::operator()() -> n8 {
  return (  //d0-d2 are reserved; always return zero
    vppLow         << 3
  | writeFailed    << 4
  | eraseFailed    << 5
  | eraseSuspended << 6
  | ready          << 7
  );
}

//

auto BSMemoryCartridge::Global::Status::operator()() -> n8 {
  return (
    page          << 0
  | pageReady     << 1
  | pageAvailable << 2
  | queueFull     << 3
  | sleeping      << 4
  | failed        << 5
  | suspended     << 6
  | ready         << 7
  );
}

//

auto BSMemoryCartridge::Queue::flush() -> void {
  history[0] = {};
  history[1] = {};
  history[2] = {};
  history[3] = {};
}

auto BSMemoryCartridge::Queue::pop() -> void {
  if(history[3].valid) { history[3] = {}; return; }
  if(history[2].valid) { history[2] = {}; return; }
  if(history[1].valid) { history[1] = {}; return; }
  if(history[0].valid) { history[0] = {}; return; }
}

auto BSMemoryCartridge::Queue::push(n24 address, n8 data) -> void {
  if(!history[0].valid) { history[0] = {true, address, data}; return; }
  if(!history[1].valid) { history[1] = {true, address, data}; return; }
  if(!history[2].valid) { history[2] = {true, address, data}; return; }
  if(!history[3].valid) { history[3] = {true, address, data}; return; }
}

auto BSMemoryCartridge::Queue::size() -> u32 {
  if(history[3].valid) return 4;
  if(history[2].valid) return 3;
  if(history[1].valid) return 2;
  if(history[0].valid) return 1;
  return 0;
}

auto BSMemoryCartridge::Queue::address(u32 index) -> n24 {
  if(index > 3 || !history[index].valid) return 0;
  return history[index].address;
}

auto BSMemoryCartridge::Queue::data(u32 index) -> n8 {
  if(index > 3 || !history[index].valid) return 0;
  return history[index].data;
}
