#pragma once
#include "state-snapshot.hpp"
// Uploads contain no core mutation. A single emulation-thread commit preflights
// the complete file against the current serializer/configuration before restore.
struct StateLoader {
  using Clock = std::chrono::steady_clock;
  struct Upload {
    uint64_t id; std::string owner; Clock::time_point deadline;
    std::vector<uint8_t> bytes; uint64_t written = 0, offset = 0, ramSize = 0;
    std::string romHash;
  };
  std::optional<Upload> upload;
  uint64_t nextId = 1;
  bool touched = false;
  void reset() { upload.reset(); }
  void releaseOwner(const std::string& owner) { if(upload && upload->owner == owner) reset(); }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args, const std::string& owner, bool available, bool busy, bool& paused, StateSnapshot& snapshots) {
    using namespace ares::Nintendo64;
    touched = false;
    if(!available) return {"unavailable", {}};
    if(upload && Clock::now() >= upload->deadline) reset();
    if(method == "n64.state.loadBegin") {
      if(!args.empty()) return {"invalid_request", {}};
      if(busy || upload || nall::GDB::server.hasClient()) return {"busy", {}};
      paused = true;
      upload = Upload{nextId++, owner, Clock::now() + std::chrono::seconds(30), {}, 0, 0, 0, {}};
      return {{}, "{\"upload_id\":" + std::to_string(upload->id) + "}"};
    }
    uint64_t id;
    if(args.empty() || !ExecutionService::number(args[0], id) || !id) return {"invalid_request", {}};
    if(!upload || upload->id != id) return {"stale_upload", {}};
    if(upload->owner != owner) return {"busy", {}};
    if(method == "n64.state.loadAbort") {
      if(args.size() != 1) return {"invalid_request", {}};
      reset(); return {{}, "{\"released\":true}"};
    }
    if(method == "n64.state.loadPrepare") {
      uint64_t bytes, offset, size;
      if(args.size() != 5 || !ExecutionService::number(args[1], bytes) || bytes < 533 || bytes > (64ull << 20) || !ExecutionService::number(args[2], offset) || !ExecutionService::number(args[3], size) ||
        (args[4] != "-" && (args[4].size() != 64 || args[4].find_first_not_of("0123456789abcdef") != std::string::npos)) ||
        (size && (offset < 533 || size > (8ull << 20) || size % 4 || offset > bytes || size > bytes - offset))) return {"invalid_request", {}};
      if(!upload->bytes.empty()) return {"invalid_request", {}};
      upload->bytes.resize(bytes); upload->offset = offset; upload->ramSize = size; upload->romHash = args[4];
      return {{}, "{\"prepared\":true}"};
    }
    if(method == "n64.state.loadWrite") {
      uint64_t offset;
      if(args.size() != 3 || !ExecutionService::number(args[1], offset) || args[2].empty() || args[2].size() % 2 || args[2].size() > 3600) return {"invalid_request", {}};
      auto count = args[2].size() / 2;
      if(upload->bytes.empty() || offset != upload->written || offset > upload->bytes.size() || count > upload->bytes.size() - offset) return {"out_of_bounds", {}};
      for(char c : args[2]) if(MemoryService::nibble(c) < 0) return {"invalid_request", {}};
      for(size_t n = 0; n < count; n++) upload->bytes[offset+n] = MemoryService::nibble(args[2][n*2]) << 4 | MemoryService::nibble(args[2][n*2+1]);
      upload->written += count;
      return {{}, "{\"written\":" + std::to_string(count) + "}"};
    }
    if(method != "n64.state.loadCommit" || args.size() != 3 || (args[1] != "0" && args[1] != "1") || (args[2] != "0" && args[2] != "1")) return {"invalid_request", {}};
    if(busy || nall::GDB::server.hasClient()) return {"busy", {}};
    paused = true;
    if(upload->bytes.empty() || upload->written != upload->bytes.size()) return {"state_truncated", {}};
    auto& bytes = upload->bytes;
    uint32_t signature = uint32_t(bytes[0]) | uint32_t(bytes[1]) << 8 | uint32_t(bytes[2]) << 16 | uint32_t(bytes[3]) << 24;
    if(signature != ares::SerializerSignature || bytes[4] > 1) return {"bad_signature", {}};
    char version[17] = {}; std::memcpy(version, bytes.data() + 5, 16);
    if(nall::string{version} != ares::Nintendo64::system.stateVersion()) return {"unsupported_version", {}};
    auto [offset, size] = ares::Nintendo64::system.inspectStateRdramLayout();
    if(upload->ramSize && (upload->offset != offset || upload->ramSize != size)) return {"state_configuration_mismatch", {}};
    if(upload->romHash != "-") {
      if(snapshots.romHash.empty()) snapshots.romHash = nall::Hash::SHA256({cartridge.rom.data, cartridge.rom.size}).digest().data();
      if(snapshots.romHash != upload->romHash) return {"state_rom_mismatch", {}};
    }
    auto current = ares::Nintendo64::system.serialize();
    if(bytes.size() != current.size()) return {"state_configuration_mismatch", {}};
    nall::serializer state{bytes.data(), uint32_t(bytes.size())};
    touched = true;
    bool loaded = ares::Nintendo64::system.unserialize(state);
    if(!loaded || state.size() != bytes.size()) {
      nall::serializer backup{current.data(), current.size()};
      bool restored = ares::Nintendo64::system.unserialize(backup) && backup.size() == current.size();
      reset(); return {restored ? "state_load_failed_restored" : "state_restore_failed", {}};
    }
    auto count = bytes.size(); reset();
    return {{}, "{\"size\":" + std::to_string(count) + "}"};
  }
};
