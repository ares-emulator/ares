#include <ares/ares.hpp>
#include "ncr53cf96.hpp"

namespace ares {

#include "serialization.cpp"

auto NCR53CF96::power() -> void {
  fifoRead = fifoWrite = fifoCount = 0;
  counter = counterStart = 0;
  command = 0;
  intr = 0;
  seqStep = 0;
  busID = 0;
  timeout = 0;
  syncPeriod = syncOffset = 0;
  config1 = config2 = config3 = config4 = 0;
  clockConversion = 0;
  test = 0;
  phase = BusFree;
  target = 0;
  selectMode = 0;
  dma = false;
  transferring = false;
  terminalCount = false;
  messageCommandComplete = false;
  for(auto* device : devices) if(device) device->power();
  updateIRQ();
}

auto NCR53CF96::updateIRQ() -> void {
  if(irq) irq(intr != 0);
}

auto NCR53CF96::raise(n8 flags) -> void {
  intr |= flags;
  updateIRQ();
}

auto NCR53CF96::pushFIFO(n8 data) -> void {
  if(fifoCount >= 16) return;
  fifo[fifoWrite] = data;
  fifoWrite = (fifoWrite + 1) & 15;
  fifoCount++;
}

auto NCR53CF96::popFIFO() -> n8 {
  if(!fifoCount) return 0;
  n8 data = fifo[fifoRead];
  fifoRead = (fifoRead + 1) & 15;
  fifoCount--;
  return data;
}

auto NCR53CF96::phaseBits() const -> n8 {
  switch(phase) {
  case DataOut: return 0;
  case DataIn: return 1;
  case Command: return 2;
  case Status: return 3;
  case MessageOut: return 6;
  case MessageIn: return 7;
  }
  return 0;
}

auto NCR53CF96::read(u32 address) -> n8 {
  switch(address & 15) {
  case 0x0: return counter >> 0;
  case 0x1: return counter >> 8;
  case 0x2: return popFIFO();
  case 0x3: return command;
  case 0x4: return phaseBits() | (terminalCount ? 0x10 : 0) | (intr ? 0x80 : 0);
  case 0x5: {
    n8 value = intr;
    intr = 0;
    seqStep = seqStep;
    updateIRQ();
    return value;
  }
  case 0x6: return seqStep & 7;
  case 0x7: return fifoCount & 0x1f | (seqStep & 7) << 5;
  case 0x8: return config1;
  case 0xa: return test;
  case 0xb: return config2;
  case 0xc: return config3;
  case 0xd: return config4;
  case 0xe: return counter >> 16;
  }
  return 0;
}

auto NCR53CF96::write(u32 address, n8 data) -> void {
  switch(address & 15) {
  case 0x0: counter = counterStart = (counter & 0xffff00) | data; terminalCount = false; break;
  case 0x1: counter = counterStart = (counter & 0xff00ff) | data << 8; terminalCount = false; break;
  case 0x2: pushFIFO(data); break;
  case 0x3: executeCommand(data); break;
  case 0x4: busID = data & 7; break;
  case 0x5: timeout = data; break;
  case 0x6: syncPeriod = data & 31; break;
  case 0x7: syncOffset = data & 15; break;
  case 0x8: config1 = data; break;
  case 0x9: clockConversion = data & 7; break;
  case 0xa: test = data; break;
  case 0xb: config2 = data; break;
  case 0xc: config3 = data; break;
  case 0xd: config4 = data; break;
  case 0xe: counter = counterStart = (counter & 0x00ffff) | data << 16; terminalCount = false; break;
  }
}

auto NCR53CF96::nextPhase() -> void {
  auto* device = devices[target];
  if(!device) { phase = BusFree; return; }
  if(device->dataOutExpected()) phase = DataOut;
  else if(device->dataInAvailable()) phase = DataIn;
  else phase = Status;
}

auto NCR53CF96::countTransfer() -> bool {
  counter = (counter - 1) & 0xffffff;
  if(counter == 0) terminalCount = true;
  return counter == 0;
}

auto NCR53CF96::executeCommand(n8 value) -> void {
  command = value;
  dma = value & 0x80;
  n8 op = value & 0x7f;

  if(dma && op != 0) {
    counter = counterStart;
    terminalCount = false;
    if(counter == 0) counter = 0x10000;
  }

  switch(op) {
  case 0x00: break;
  case 0x01: fifoRead = fifoWrite = fifoCount = 0; break;
  case 0x02: {
    auto savedIRQ = irq;
    auto saved = std::move(devices);
    (void)saved;
    fifoRead = fifoWrite = fifoCount = 0;
    counter = counterStart = 0;
    intr = 0;
    seqStep = 0;
    config1 = config2 = config3 = config4 = 0;
    phase = BusFree;
    selectMode = 0;
    transferring = false;
    terminalCount = false;
    updateIRQ();
    break;
  }
  case 0x03:
    phase = BusFree;
    selectMode = 0;
    transferring = false;
    for(auto* device : devices) if(device) device->power();
    if(!(config1 & 0x40)) raise(0x80);
    break;
  case 0x41:
  case 0x42:
  case 0x43: {
    selectMode = op - 0x40 + 1;
    if(dma) {
      counter = counterStart ? counterStart : 0;
      if(counter == 0) {
        selectMode = 0;
        select(op != 0x41, op == 0x43);
      }
    } else {
      selectMode = 0;
      select(op != 0x41, op == 0x43);
    }
    break;
  }
  case 0x44:
  case 0x45:
    break;
  case 0x10: transferInformation(); break;
  case 0x11:
    if(phase == Status) {
      auto* device = devices[target];
      pushFIFO(device ? (n8)device->status : (n8)0);
      pushFIFO(0);
      phase = MessageIn;
      messageCommandComplete = true;
      raise(0x08);
    } else raise(0x40);
    break;
  case 0x12:
    if(phase == MessageIn) {
      phase = BusFree;
      raise(messageCommandComplete ? 0x20 : 0x10);
      messageCommandComplete = false;
    } else raise(0x40);
    break;
  case 0x18: raise(0x08); break;
  case 0x1a:
  case 0x1b: break;
  default: raise(0x40); break;
  }
}

auto NCR53CF96::select(bool atn, bool stop) -> void {
  u32 id = busID & 7;
  auto* device = devices[id];
  if(!device) {
    fifoRead = fifoWrite = fifoCount = 0;
    seqStep = 0;
    phase = BusFree;
    raise(0x20);
    return;
  }

  if(atn) {
    if(!fifoCount) return raise(0x40);
    popFIFO();
  }

  target = id;
  if(atn && stop) {
    phase = Command;
    seqStep = 1;
    raise(0x18);
    return;
  }

  if(!fifoCount) {
    phase = Command;
    seqStep = 2;
    raise(0x18);
    return;
  }

  u32 length = SCSIDevice::commandLength(fifo[fifoRead]);
  std::vector<u8> cdb;
  while(fifoCount && cdb.size() < length) cdb.push_back(popFIFO());
  fifoRead = fifoWrite = fifoCount = 0;
  device->execute(cdb);
  seqStep = 4;
  nextPhase();
  raise(0x18);
}

auto NCR53CF96::transferInformation() -> void {
  auto* device = devices[target];
  if(!device) { phase = BusFree; return raise(0x20); }

  switch(phase) {
  case DataIn:
    if(dma) {
      transferring = true;
      return;
    }
    while(device->dataInAvailable() && fifoCount < 16 && (counter == 0 || !terminalCount)) {
      pushFIFO(device->dataIn());
      if(counter) countTransfer();
    }
    if(!device->dataInAvailable()) { nextPhase(); raise(0x10); }
    else raise(0x08);
    return;

  case DataOut:
    if(dma) {
      transferring = true;
      return;
    }
    while(fifoCount && device->dataOutExpected()) device->dataOut(popFIFO());
    nextPhase();
    raise(0x10);
    return;

  case Command: {
    u32 length = fifoCount ? SCSIDevice::commandLength(fifo[fifoRead]) : 6;
    std::vector<u8> cdb;
    while(fifoCount && cdb.size() < length) cdb.push_back(popFIFO());
    device->execute(cdb);
    seqStep = 4;
    nextPhase();
    raise(0x18);
    return;
  }

  case MessageOut:
    popFIFO();
    phase = Command;
    raise(0x18);
    return;

  case MessageIn:
    pushFIFO(0);
    raise(0x08);
    return;

  case Status:
    pushFIFO(device->status);
    phase = MessageIn;
    messageCommandComplete = true;
    raise(0x08);
    return;
  }

  raise(0x40);
}

auto NCR53CF96::canDMARead() const -> bool {
  return transferring && phase == DataIn;
}

auto NCR53CF96::canDMAWrite() const -> bool {
  return selectMode != 0 || (transferring && phase == DataOut);
}

auto NCR53CF96::dmaRead() -> n8 {
  auto* device = devices[target];
  if(!transferring || phase != DataIn || !device) return 0;
  n8 data = device->dataIn();
  bool zero = countTransfer();
  if(!device->dataInAvailable()) {
    transferring = false;
    nextPhase();
    raise(0x10);
  } else if(zero) {
    transferring = false;
    raise(0x08);
  }
  return data;
}

auto NCR53CF96::dmaWrite(n8 data) -> void {
  if(selectMode) {
    pushFIFO(data);
    if(countTransfer()) {
      n8 mode = selectMode;
      selectMode = 0;
      select(mode != 1, mode == 3);
    }
    return;
  }
  auto* device = devices[target];
  if(!transferring || phase != DataOut || !device) return;
  device->dataOut(data);
  bool zero = countTransfer();
  if(!device->dataOutExpected()) {
    transferring = false;
    nextPhase();
    raise(0x10);
  } else if(zero) {
    transferring = false;
    raise(0x08);
  }
}

}
