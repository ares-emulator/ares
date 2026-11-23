#pragma once

#include "../scsi.hpp"

namespace ares {

struct SCSICDROM : SCSIDevice {
  struct Identity {
    string vendor;
    string product;
    string revision;
  };

  std::function<CD::Session*()> session;
  std::function<bool(s32 lba, u8* sector)> readSector;

  auto setIdentity(string vendor, string product, string revision) -> void;

  auto power() -> void override;
  auto execute(std::span<const u8> cdb) -> void override;
  auto dataInAvailable() -> bool override;
  auto dataIn() -> n8 override;
  auto dataOutExpected() -> bool override;
  auto dataOut(n8 data) -> void override;
  auto serialize(serializer&) -> void override;

  auto ready() -> bool;
  auto checkCondition(n8 key, n8 asc, n8 ascq = 0) -> void;
  auto respond(std::vector<u8> data, u32 limit) -> void;
  auto loadSector() -> bool;
  auto readBlocks(s32 lba, u32 count) -> void;
  auto readToc(std::span<const u8> cdb) -> void;
  auto readSubchannel(std::span<const u8> cdb) -> void;
  auto playAudio(s32 start, s32 end) -> void;
  auto audioTick() -> void;
  auto modeSense(std::span<const u8> cdb) -> void;

  Identity identity = {"GENERIC ", "CD-ROM", "1.0"};
  std::vector<u8> buffer;
  u32 position = 0;
  s32 readLBA = 0;
  u32 readRemaining = 0;
  u8 raw[2352] = {};
  u32 sectorOffset = 0;
  u32 sectorLength = 0;
  u32 sectorPosition = 0;
  u32 blockSize = 2048;
  n8 audioStatus = 0x15;
  s32 audioPosition = 0;
  s32 audioEnd = 0;
  n8 senseKey = 0;
  n8 senseASC = 0;
  n8 senseASCQ = 0;
  u32 outRemaining = 0;
  std::vector<u8> outBuffer;
  bool outModeSelect = false;
};

}
