#pragma once
#include "machine-service.hpp"
#include "execution-service.hpp"
#include <optional>
// Safe-point snapshots/observations adapted from ares-64 telemetry-server.cpp
// at 1c9b9637476a83f7b80d3db1115a0f71529820d9. No native filesystem access.
struct InspectionService {
  using Clock = ExecutionService::Clock;
  struct Pin { uint64_t id; std::string owner; std::vector<uint8_t> bytes; Clock::time_point expires; };
  struct Baseline { uint64_t epoch, frame; uint32_t origin, dpc; };
  std::optional<Pin> pin;
  std::unordered_map<std::string, Baseline> baselines;
  uint64_t nextId = 1;
  void reset() { pin.reset(); baselines.clear(); }
  void releaseOwner(const std::string& owner) { if(pin && pin->owner == owner) pin.reset(); baselines.erase(owner); }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args, const std::string& owner, uint64_t epoch, uint64_t frame, bool available, bool busy, bool paused) {
    using namespace ares::Nintendo64;
    if(!available) return {"unavailable", {}};
    if(pin && Clock::now() >= pin->expires) pin.reset();
    if(method == "n64.render.probe") {
      if(!args.empty()) return {"invalid_request", {}};
      auto previous = baselines.find(owner);
      if(previous == baselines.end() && baselines.size() >= 16) return {"busy", {}};
      Baseline now{epoch, frame, (uint32_t)vi.io.dramAddress, (uint32_t)rdp.command.clock};
      bool have = previous != baselines.end() && previous->second.epoch == epoch;
      bool active = vi.active(), origin = have && previous->second.origin != now.origin;
      bool dpc = have && previous->second.dpc != now.dpc, advanced = have && previous->second.frame != frame;
      MachineService::Object out;
      out.add("viActive", active ? "true" : "false"); out.hex("viOrigin", now.origin); out.hex("dpcClock", now.dpc);
      out.number("epoch", epoch); out.number("frame", frame); out.add("paused", paused ? "true" : "false");
      out.add("presented_frame", "null"); out.add("vps", "null");
      out.add("originChanged", !have ? "null" : origin ? "true" : "false");
      out.add("dpcAdvanced", !have ? "null" : dpc ? "true" : "false");
      out.add("frameAdvanced", !have ? "null" : advanced ? "true" : "false");
      out.add("rendering", !have ? "null" : active && origin && dpc && advanced ? "true" : "false");
      baselines[owner] = now; return {{}, out.end()};
    }
    if(method == "n64.dump.begin") {
      uint64_t address, length, expected; uint32_t physical; nall::string error;
      if(args.size() != 4 || (args[0] != "all" && args[0] != "cpu" && args[0] != "rdram") || !N64Service::address(args[1], address)
        || !ExecutionService::number(args[2], length) || !ExecutionService::number(args[3], expected)) return {"invalid_request", {}};
      bool all = args[0] == "all";
      if(all && (address || length)) return {"invalid_request", {}};
      if(!all && (expected || !length || length > 1048576)) return {"invalid_request", {}};
      if(all && expected && expected != rdram.ram.size) return {"size_mismatch", {}};
      if(busy || pin || nall::GDB::server.hasClient()) return {"busy", {}};
      if(all) { address = 0; length = rdram.ram.size; }
      if(!length || length > 8388608 || !cpu.debugControl.normalizeAddress(address, length, physical, error)) return {"out_of_bounds", {}};
      ares::Nintendo64::system.prepareVideo();
#if defined(VULKAN)
      if(vulkan.enable) vulkan.flush();
#endif
      auto canonical = MachineService::canonical(address);
      bool cached = args[0] == "cpu" && !(canonical >= 0xffffffffa0000000ull && canonical < 0xffffffffc0000000ull);
      pin = Pin{nextId++, owner, std::vector<uint8_t>(length), Clock::now() + std::chrono::seconds(30)};
      for(uint32_t i = 0; i < length; ++i) pin->bytes[i] = cached ? cpu.dcache.readDebug<Byte>(0xffffffff80000000ull | (physical + i), physical + i)
        : rdram.ram.read<Byte>(physical + i, RBusDevice::ARES_DEBUGGER);
      MachineService::Object out; out.number("snapshot_id", pin->id); out.number("bytes", length); out.number("epoch", epoch); out.number("frame", frame);
      out.hex("addr", canonical); out.hex("paddr", physical); out.add("view", controlQuote(cached ? "cpu" : args[0] == "cpu" ? "cpu" : "rdram"));
      out.add("cache_aware", cached ? "true" : "false"); out.add("stopped", paused || cpu.debugControl.halted ? "true" : "false");
      return {{}, out.end()};
    }
    if(method != "n64.dump.read" && method != "n64.dump.release") return {"unavailable", {}};
    uint64_t id, offset = 0, length = 0;
    if(args.size() != (method == "n64.dump.read" ? 3 : 1) || !ExecutionService::number(args[0], id) || !id ||
      (method == "n64.dump.read" && (!ExecutionService::number(args[1], offset) || !ExecutionService::number(args[2], length) || !length || length > 16384))) return {"invalid_request", {}};
    if(!pin || pin->id != id) return {"stale_snapshot", {}};
    if(pin->owner != owner) return {"busy", {}};
    if(method == "n64.dump.release") { pin.reset(); return {{}, "{\"released\":true}"}; }
    if(offset >= pin->bytes.size() || length > pin->bytes.size() - offset) return {"out_of_bounds", {}};
    std::string hex; hex.reserve(length * 2); static const char h[] = "0123456789abcdef";
    for(uint64_t i = offset; i < offset + length; ++i) { auto b = pin->bytes[i]; hex += h[b >> 4]; hex += h[b & 15]; }
    return {{}, "{\"snapshot_id\":" + std::to_string(id) + ",\"offset\":" + std::to_string(offset) + ",\"length\":" + std::to_string(length) + ",\"hex\":" + controlQuote(hex) + "}"};
  }
};
