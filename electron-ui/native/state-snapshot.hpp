#pragma once
#include "n64-service.hpp"
#include "execution-service.hpp"
#include <optional>
#include <nall/hash/sha256.hpp>
// Emulation-thread immutable serialization. Paths/publication stay in main.
struct StateSnapshot {
  using Clock = std::chrono::steady_clock;
  struct Pin { uint64_t id; std::string owner; std::vector<uint8_t> bytes; Clock::time_point expires; };
  std::optional<Pin> pin;
  uint64_t nextId = 1;
  std::string romHash;
  void reset() { pin.reset(); }
  void releaseOwner(const std::string& owner) { if(pin && pin->owner == owner) pin.reset(); }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args, const std::string& owner, uint64_t epoch, uint64_t frame, bool available, bool busy, bool& paused) {
    using namespace ares::Nintendo64;
    if(!available) return {"unavailable", {}};
    if(pin && Clock::now() >= pin->expires) pin.reset();
    if(method == "n64.state.begin") {
      if(!args.empty()) return {"invalid_request", {}};
      if(busy || nall::GDB::server.hasClient() || pin) return {"busy", {}};
      paused = true;
      auto state = ares::Nintendo64::system.serialize();
      if(state.size() < 533 || state.size() > (64ull << 20)) return {"state_too_large", {}};
      auto [offset, size] = ares::Nintendo64::system.inspectStateRdramLayout();
      if(offset > state.size() || size > state.size() - offset) return {"invalid_layout", {}};
      if(romHash.empty()) romHash = nall::Hash::SHA256({cartridge.rom.data, cartridge.rom.size}).digest().data();
      pin = Pin{nextId++, owner, {state.data(), state.data() + state.size()}, Clock::now() + std::chrono::seconds(30)};
      return {{}, "{\"snapshot_id\":" + std::to_string(pin->id) + ",\"bytes\":" + std::to_string(pin->bytes.size()) + ",\"version\":" + controlQuote(ares::Nintendo64::system.stateVersion().data())
        + ",\"epoch\":" + std::to_string(epoch) + ",\"frame\":" + std::to_string(frame) + ",\"offset\":" + std::to_string(offset) + ",\"size\":" + std::to_string(size)
        + ",\"storage\":\"lsb32\",\"rom_sha256\":" + controlQuote(romHash) + ",\"pc\":" + controlQuote(N64Service::hex(cpu.pipeline.pc))
        + ",\"stopped\":true,\"mid_frame_cpu_stop\":" + (cpu.debugControl.halted ? "true" : "false") + "}"};
    }
    if(method != "n64.state.read" && method != "n64.state.release") return {"unavailable", {}};
    uint64_t id, offset = 0, length = 0;
    if(args.size() != (method == "n64.state.read" ? 3 : 1) || !ExecutionService::number(args[0], id) || !id ||
      (method == "n64.state.read" && (!ExecutionService::number(args[1], offset) || !ExecutionService::number(args[2], length) || !length || length > 16384))) return {"invalid_request", {}};
    if(!pin || pin->id != id) return {"stale_snapshot", {}};
    if(pin->owner != owner) return {"busy", {}};
    if(method == "n64.state.release") { pin.reset(); return {{}, "{\"released\":true}"}; }
    if(offset >= pin->bytes.size() || length > pin->bytes.size() - offset) return {"out_of_bounds", {}};
    std::string hex; hex.reserve(length * 2); static const char digits[] = "0123456789abcdef";
    for(uint64_t n = offset; n < offset + length; n++) { auto byte = pin->bytes[n]; hex += digits[byte >> 4]; hex += digits[byte & 15]; }
    return {{}, "{\"snapshot_id\":" + std::to_string(id) + ",\"offset\":" + std::to_string(offset) + ",\"length\":" + std::to_string(length) + ",\"hex\":" + controlQuote(hex) + "}"};
  }
};
