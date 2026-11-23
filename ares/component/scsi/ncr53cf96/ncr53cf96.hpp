#pragma once

#include "../scsi.hpp"

namespace ares {

struct NCR53CF96 {
  std::function<void(bool)> irq;
  SCSIDevice* devices[8] = {};

  enum Phase : u32 { BusFree, DataOut, DataIn, Command, Status, MessageOut, MessageIn };

  auto power() -> void;
  auto read(u32 address) -> n8;
  auto write(u32 address, n8 data) -> void;
  auto canDMARead() const -> bool;
  auto canDMAWrite() const -> bool;
  auto dmaRead() -> n8;
  auto dmaWrite(n8 data) -> void;
  auto serialize(serializer&) -> void;

  auto executeCommand(n8 value) -> void;
  auto select(bool atn, bool stop) -> void;
  auto transferInformation() -> void;
  auto raise(n8 flags) -> void;
  auto updateIRQ() -> void;
  auto pushFIFO(n8 data) -> void;
  auto popFIFO() -> n8;
  auto phaseBits() const -> n8;
  auto finishDataIn() -> void;
  auto countTransfer() -> bool;
  auto nextPhase() -> void;

  n8 fifo[16] = {};
  u32 fifoRead = 0;
  u32 fifoWrite = 0;
  u32 fifoCount = 0;

  u32 counter = 0;
  n8 command = 0;
  n8 intr = 0;
  n8 seqStep = 0;
  n8 busID = 0;
  n8 timeout = 0;
  n8 syncPeriod = 0;
  n8 syncOffset = 0;
  n8 config1 = 0;
  n8 config2 = 0;
  n8 config3 = 0;
  n8 config4 = 0;
  n8 clockConversion = 0;
  n8 test = 0;
  n8 phase = BusFree;
  n8 target = 0;
  u32 counterStart = 0;
  n8 selectMode = 0;
  bool dma = false;
  bool transferring = false;
  bool terminalCount = false;
  bool messageCommandComplete = false;
};

}
