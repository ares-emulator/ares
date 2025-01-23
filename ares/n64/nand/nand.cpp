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

  auto nandSize = system.is_128 ? 128_MiB : 64_MiB;
  auto spareSize = nandSize / 32;

  data.allocate(nandSize);
  spare.allocate(spareSize);
  for(auto i : range(NAND::NUM_PLANES)) {
    writeBuffers[i].allocate(0x200);
    writeBufferSpares[i].allocate(0x10);
  }
  debugger.load(node);
}

auto NAND::unload() -> void {
  debugger.unload(node);
  node.reset();

  data.reset();
  spare.reset();
  for(auto i : range(NAND::NUM_PLANES)) {
    writeBuffers[i].reset();
    writeBufferSpares[i].reset();
  }
}

auto NAND::save() -> void {
  if(!node) return;

  if (num != 0) return; //TODO: multi-NAND

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
  if (num != 0) return; //TODO: multi-NAND

  // open NAND image
  string dataName = "nand.flash";
  if(auto fp = system.pak->write(dataName)) {
    data.load(fp);
  }
  string spareName = "spare.flash";
  if(auto fp = system.pak->write(spareName)) {
    spare.load(fp);
  }

  for(auto i : range(NAND::NUM_PLANES)) {
    writeBuffers[i].fill();
    writeBufferSpares[i].fill();
  }
}

auto NAND::read(Memory::Writable& dest, b1 which, n27 nandAddr, n10 length, b1 ecc) -> n2 {
  // Fill up to pageOffset with 0
  for (u32 i = 0; i < pageOffset; i++)
    dest.write<Byte>(0x200 * which + i, 0);

  // Fill pageOffset to end of data with data
  for (u32 i = pageOffset; i < min(pageOffset + length, 0x200); i++)
    dest.write<Byte>(0x200 * which + i, data.read<Byte>(nandAddr + i));

  n2 result = 0;

  if (pageOffset + length > 0x200) {
    // Read the requested amount of spare data
    auto spareAddr = (nandAddr / 0x200) * 0x10;

    for (auto i : range(pageOffset + length - 0x200))
      dest.write<Byte>(0x400 + 0x10 * which + i, spare.read<Byte>(spareAddr + i));

    // Do ECC verification and correction if requested
    // TODO: should this be outside of the above if, or does it require ecc to be enabled?
    // It's certainly meaningless if the spare data was not read
    if (ecc) result = ECC::checkPageECC(dest, nandAddr, which, pageOffset);
  }

  string message = { "Buffer=", which, ", PageAddr=0x", hex(nandAddr), ", Length=0x", hex(length) };
  debugger.command(NAND::Command::Read0, message);
  return result;
}

auto NAND::readId(Memory::Writable& dest, b1 which, n10 length) -> void {
  for (auto i : range(length))
    dest.write<Byte>(i + which * 0x200, (i > 4) ? 0 : (system.is_128 ? system.nand128[i] : system.nand64[i]));

  string message = { "Buffer=", which, ", Length=0x", hex(length) };
  debugger.command(NAND::Command::ReadID, message);
}

auto NAND::writeToBuffer(Memory::Writable& src, b1 which, n27 nandAddr, n10 length) -> void {
  // Transfers data from PI buffer to the internal write buffer for the plane that this address belongs to
  auto plane = (nandAddr & 0xC000) >> 14;

  // Transfer data to appropriate write buffer at pageOffset
  for (auto i : range(min(length, 0x200)))
    writeBuffers[plane].write<Byte>(pageOffset + i, src.read<Byte>(i + which * 0x200));

  if (pageOffset + length > 0x200) {
    // Transfer spare to appropriate write buffer
    u8 recalc = 0xFF;
    for (auto i : range(pageOffset + length - 0x200)) {
      u8 data = src.read<Byte>(i + which * 0x10 + 0x400);
      writeBufferSpares[plane].write<Byte>(i, data);
      recalc &= data;
    }
    // If spare was all FF, recalculate ecc
    if (recalc == 0xFF) ECC::computePageECC(writeBuffers[plane], writeBufferSpares[plane]);
  }

  writeBufferAddrs[plane] = nandAddr;

  string message = { "Buffer=", which, ", PageAddr=0x", hex(nandAddr), ", Length=0x", hex(length) };
  debugger.command(NAND::Command::PageProgramC1, message);
}

auto NAND::queueWriteBuffer(n27 nandAddr, bool commit) -> void {
  // Queues this buffer for programming on next commitWriteBuffer
  auto plane = (nandAddr & 0xC000) >> 14;

  if (writeBufferAddrs[plane] != nandAddr) return;

  writeBuffersOccupied.bit(plane) = 1;

  if (commit) return;
  string message = { "Addr=0x", hex(nandAddr) };
  debugger.command(NAND::Command::PageProgramDummyC2, message);
}

auto NAND::commitWriteBuffer(n27 nandAddr) -> void {
  // Queue a write with this address
  queueWriteBuffer(nandAddr);

  string message = { "Addr=0x", hex(nandAddr), " | Queue=", binary(u32(writeBuffersOccupied), 4, '0') };
  debugger.command(NAND::Command::PageProgramC2, message);

  // Transfer all queued write buffers to the NAND flash itself
  for (auto plane : range(NAND::NUM_PLANES)) {
    if (!writeBuffersOccupied.bit(plane))
      continue;

    Memory::Writable& writeBuffer = writeBuffers[plane];
    Memory::Writable& writeBufferSpare = writeBufferSpares[plane];
    auto addr = writeBufferAddrs[plane];
    auto spareAddr = (addr / 0x200) * 0x10;

    // Transfer from write buffer to NAND, only flips 1 -> 0, 0 remains 0, respects pageOffset for partial page programming
    for (auto i : range(pageOffset, 0x200 - pageOffset))
      data.write<Byte>(addr + i, data.read<Byte>(addr + i) & writeBuffer.read<Byte>(i));
    for (auto i : range(0x10))
      spare.write<Byte>(spareAddr + i, spare.read<Byte>(spareAddr + i) & writeBufferSpare.read<Byte>(i));

    writeBuffersOccupied.bit(plane) = 0;
  }
}

auto NAND::queueErasure(n27 nandAddr) -> void {
  // Queue erasure of the block containing nandAddr

  // Index by plane, plane = block % 4
  auto plane = (nandAddr & 0xC000) >> 14;
  // Add block number to erase
  eraseQueueAddrs[plane] = nandAddr & ~(0x4000-1);
  eraseQueueOccupied.bit(plane) = 1;

  string message = { "Addr=0x", hex(nandAddr) };
  debugger.command(NAND::Command::BlockEraseC1, message);
}

auto NAND::execErasure() -> void {
  string message = { "Queue=", binary(u32(eraseQueueOccupied), 4, '0') };
  debugger.command(NAND::Command::BlockEraseC2, "");

  for (auto plane : range(NAND::NUM_PLANES)) {
    if (!eraseQueueOccupied.bit(plane))
      continue;

    auto nandAddr = eraseQueueAddrs[plane];
    auto spareAddr = (nandAddr / 0x200) * 0x10;

    // Erase the block
    for (auto j : range(0x4000/4))
      data.write<Word>(nandAddr + j * 4, 0xFFFFFFFF);
    // Erase the spare data for the pages in the block
    for (auto j : range(32 * 0x10/4))
      spare.write<Word>(spareAddr + j * 4, 0xFFFFFFFF);

    eraseQueueOccupied.bit(plane) = 0;
  }
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
