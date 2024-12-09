#include <n64/n64.hpp>

namespace ares::Nintendo64 {

NAND nand0(0);
NAND nand1(1);
NAND nand2(2);
NAND nand3(3);
#include "debugger.cpp"
#include "serialization.cpp"

auto NAND::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("NAND");

  data.allocate(0x4000000);
  spare.allocate(0x10000);
  writeBuffer.allocate(0x200);
  writeBufferSpare.allocate(0x10);

  debugger.load(node);
}

auto NAND::unload() -> void {
  debugger = { .num = debugger.num, };
  node.reset();
  
  data.reset();
  spare.reset();
  writeBuffer.reset();
  writeBufferSpare.reset();
}

auto NAND::save() -> void {
  if(!node) return;

  if (debugger.num != 0) return; //TODO: multi-NAND

  // save NAND image
  string dataName = "nand.flash";
  if(auto fp = system.pak->write(dataName)) {
    data.save(fp);
  }
  string spareName = "spare.flash";
  if(auto fp = system.pak->write(spareName)) {
    spare.save(fp);
  }
}

auto NAND::power(bool reset) -> void {
  if (debugger.num != 0) return; //TODO: multi-NAND

  // open NAND image
  string dataName = "nand.flash";
  if(auto fp = system.pak->write(dataName)) {
    data.load(fp);
  }
  string spareName = "spare.flash";
  if(auto fp = system.pak->write(spareName)) {
    spare.load(fp);
  }

  writeBuffer.fill();
  writeBufferSpare.fill();
}

auto NAND::read(Memory::Writable& dest, b1 which, n27 pageNum, n10 length) -> void {
  for (u32 i = 0; i < pageOffset; i++)
    dest.write<Byte>(i + which * 0x200, 0);

  for (u32 i = pageOffset; i < min(pageOffset + length, 0x200); i++)
    dest.write<Byte>(i + which * 0x200, data.read<Byte>(pageNum + i));

  if (pageOffset + length > 0x200) {
    for (auto i : range(pageOffset + length - 0x200)) // TODO: move to page spares
      dest.write<Byte>(i + which * 0x10 + 0x400, spare.read<Byte>(((pageNum >> 14) << 4) + i));

    // TODO: if ecc, do verification + correction here
  }

  string message = { "Buffer=", which, ", PageAddr=0x", hex(pageNum), ", Length=0x", hex(length) };
  debugger.command(NAND::Command::Read0, message);
}

auto NAND::readId(Memory::Writable& dest, b1 which, n10 length) -> void {
  for (auto i : range(length))
    dest.write<Byte>(i + which * 0x200, (i > 4) ? 0 : ID[i]);

  string message = { "Buffer=", which, ", Length=0x", hex(length) };
  debugger.command(NAND::Command::ReadID, message);
}

auto NAND::writeToBuffer(Memory::Writable& src, b1 which, n27 pageNum, n10 length) -> void {
  // transfers data from PI buffer to the internal write buffer
  for (auto i : range(min(length, 0x200)))
    writeBuffer.write<Byte>(pageOffset + i, src.read<Byte>(i + which * 0x200));

  if (length > 0x200) {
    for (auto i : range(length - 0x200))
      writeBufferSpare.write<Byte>(i, src.read<Byte>(i + which * 0x10 + 0x400));
  }
  
  //TODO: If writeBufferSpare is all FF, calculate ecc

  string message = { "Buffer=", which, ", PageAddr=0x", hex(pageNum), ", Length=0x", hex(length) };
  debugger.command(NAND::Command::PageProgramC1, message);
}

auto NAND::commitWriteBuffer(n27 pageNum) -> void {
  // transfers the write buffer to the NAND flash itself, only flips 1 -> 0, 0 remains 0
  for (auto i : range(0x200))
    data.write<Byte>(pageNum + i, data.read<Byte>(pageNum + i) & writeBuffer.read<Byte>(i));

  for (auto i : range(0x10)) // TODO: page spares
    spare.write<Byte>(((pageNum >> 14) << 4) + i, writeBufferSpare.read<Byte>(i));

  string message = { "PageAddr=0x", hex(pageNum) };
  debugger.command(NAND::Command::PageProgramC2, message);
}

auto NAND::queueErasure(n27 pageNum) -> void {
  // queue erasure of page number

  // Index by plane. plane = block % 4
  auto i = pageNum & 0xC000 >> 14;
  // Add block number to erase
  eraseQueuePage[i] = pageNum & ~(0x4000-1);
  eraseQueueOccupied.bit(i) = 1;

  string message = { "PageAddr=0x", hex(pageNum) };
  debugger.command(NAND::Command::BlockEraseC1, message);
}

auto NAND::execErasure() -> void {
  for (auto i : range(4)) {
    if (!eraseQueueOccupied.bit(i))
      continue;

    // Erase the block
    for (auto j : range(0x4000/4)) // TODO erase spare
      data.write<Word>(eraseQueuePage[i] + j * 4, 0xFFFFFFFF);

    eraseQueueOccupied.bit(i) = 0;
  }

  debugger.command(NAND::Command::BlockEraseC2, "");
}

auto NAND::readStatus(Memory::Writable& dest, b1 which, n10 length, bool multiplane) -> void {
  n8 status;

  bool fail = 0;
  bool fail_plane0 = 0;
  bool fail_plane1 = 0;
  bool fail_plane2 = 0;
  bool fail_plane3 = 0;
  bool ready_bit = 1;
  bool write_protect_disabled = 1;

  status.bit(0) = fail;
  status.bit(1) = multiplane ? fail_plane0 : 0;
  status.bit(2) = multiplane ? fail_plane1 : 0;
  status.bit(3) = multiplane ? fail_plane2 : 0;
  status.bit(4) = multiplane ? fail_plane3 : 0;
  status.bit(5) = 0;
  status.bit(6) = ready_bit;
  status.bit(7) = write_protect_disabled;

  for (auto i : range(length))
    dest.write<Byte>(i + which * 0x200, (i == 0) ? u8(status) : 0);

  string message = { "Buffer=", which, ", Length=0x", hex(length), ", Multiplane=", boolean(multiplane) };
  debugger.command(NAND::Command::ReadStatus, message);
}

}
