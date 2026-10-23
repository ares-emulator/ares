#pragma once
#include <charconv>
#include <string>
#include <vector>
#include <functional>

inline auto controlQuote(const std::string& value) -> std::string {
  std::string result = "\"";
  for(unsigned char c : value) {
    if(c == '"' || c == '\\') result += '\\';
    if(c >= 32) result += c;
  }
  return result + "\"";
}
#include "control-result.hpp"

// This service exposes debugger-region offsets, never CPU virtual addresses.
struct MemoryService {
  static constexpr uint32_t maxTransfer = 1024;
  struct Region {
    std::string id; ares::Node::Debugger::Memory node; bool supported, writesAllowed;
    std::function<uint8_t(uint32_t)> hostRead;
    uint8_t read(uint32_t offset) { return hostRead ? hostRead(offset) : uint8_t(node->read(offset)); }
  };
  std::vector<Region> regions;
  void bind(ares::Node::System root, const std::string& session, bool n64) {
    regions.clear();
    if(!root) return;
    for(auto node : root->find<ares::Node::Debugger::Memory>()) {
      // DCache has CPU-view semantics and a size independent of installed RDRAM.
      // It must not masquerade as bounded raw storage (task 5 owns that service).
      bool supported = !(n64 && node->name() == "DCache");
      regions.push_back({"r_" + session + "_" + std::to_string(regions.size()), node, supported, !(n64 && node->name() == "RDRAM")});
    }
  }
  static bool number(const std::string& text, uint32_t& value) {
    if(text.empty()) return false;
    auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    return error == std::errc{} && end == text.data() + text.size();
  }
  static int nibble(char c) {
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'a' && c <= 'f') return c - 'a' + 10;
    if(c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  }
  auto execute(const std::string& method, const std::vector<std::string>& args) -> ControlResult {
    if(method == "memory.regions") {
      if(!args.empty()) return {"invalid_request", {}};
      if(regions.size() > 128) return {"too_many_regions", {}};
      std::string json = "{\"regions\":[";
      for(auto& r : regions) {
        if(json.back() != '[') json += ',';
        json += "{\"id\":" + controlQuote(r.id) + ",\"name\":" + controlQuote(r.node->name().data()) + ",\"size\":" + std::to_string(r.node->size()) + ",\"readable\":" + (r.supported && r.node->readable() ? "true" : "false") + ",\"writable\":" + (r.supported && r.writesAllowed && r.node->writable() ? "true" : "false") + ",\"addressing\":\"region-offset\"";
        if(!r.supported) json += ",\"unavailableReason\":\"Requires N64 cache-aware service\"";
        else if(!r.writesAllowed) json += ",\"unavailableReason\":\"Writes require N64 memory service; host reads use raw physical storage\"";
        json += '}';
      }
      return {{}, json + "]}"};
    }
    if(method == "memory.readMany") {
      if(args.empty() || args.size() % 3 || args.size() > 64 * 3) return {"invalid_request", {}};
      struct Range { Region* region; uint32_t offset, length; };
      std::vector<Range> ranges;
      uint32_t total = 0;
      for(size_t i = 0; i < args.size(); i += 3) {
        uint32_t offset, length;
        if(!number(args[i + 1], offset) || !number(args[i + 2], length) || !length || length > 65536 - total) return {"invalid_request", {}};
        Region* region = nullptr;
        for(auto& r : regions) if(r.id == args[i]) { region = &r; break; }
        if(!region) return {"stale_region", {}};
        if(!region->supported) return {"unsupported_region", {}};
        if(!region->node->readable()) return {"region_not_readable", {}};
        if(offset > region->node->size() || length > region->node->size() - offset) return {"out_of_bounds", {}};
        total += length; ranges.push_back({region, offset, length});
      }
      // Every range is validated before the first callback. This loop never yields.
      std::string out = "{\"reads\":[";
      const char* digits = "0123456789abcdef";
      for(auto& r : ranges) {
        std::string bytes; bytes.reserve(r.length * 2);
        for(uint32_t i = 0; i < r.length; ++i) { uint8_t v = r.region->read(r.offset + i); bytes += digits[v >> 4]; bytes += digits[v & 15]; }
        if(out.back() != '[') out += ',';
        out += "{\"region\":" + controlQuote(r.region->id) + ",\"offset\":" + std::to_string(r.offset) + ",\"length\":" + std::to_string(r.length) + ",\"hex\":" + controlQuote(bytes) + "}";
      }
      return {{}, out + "]}"};
    }
    if(method != "memory.read" && method != "memory.write") return {"unavailable", {}};
    if(args.size() != 3) return {"invalid_request", {}};
    uint32_t offset, length;
    if(!number(args[1], offset)) return {"invalid_request", {}};
    std::vector<uint8_t> bytes;
    if(method == "memory.read") {
      if(!number(args[2], length) || !length || length > maxTransfer) return {"invalid_request", {}};
    } else {
      const auto& hex = args[2];
      if(hex.empty() || hex.size() % 2 || hex.size() > maxTransfer * 2) return {"invalid_request", {}};
      for(size_t i = 0; i < hex.size(); i += 2) {
        int a = nibble(hex[i]), b = nibble(hex[i + 1]);
        if(a < 0 || b < 0) return {"invalid_request", {}};
        bytes.push_back((a << 4) | b);
      }
      length = bytes.size();
    }
    Region* region = nullptr;
    for(auto& r : regions) if(r.id == args[0]) { region = &r; break; }
    if(!region) return {"stale_region", {}};
    auto& node = region->node;
    if(!region->supported) return {"unsupported_region", {}};
    if(offset > node->size() || length > node->size() - offset) return {"out_of_bounds", {}};
    // Validate the entire operation before invoking any callback. No wraparound,
    // read-modify-write, partial validation or fabricated writable metadata.
    std::string json = "{\"region\":" + controlQuote(region->id) + ",\"offset\":" + std::to_string(offset);
    if(method == "memory.read") {
      if(!node->readable()) return {"region_not_readable", {}};
      const char* digits = "0123456789abcdef";
      std::string hex; hex.reserve(length * 2);
      for(uint32_t i = 0; i < length; ++i) { uint8_t value = region->read(offset + i); hex += digits[value >> 4]; hex += digits[value & 15]; }
      return {{}, json + ",\"length\":" + std::to_string(length) + ",\"hex\":" + controlQuote(hex) + "}"};
    }
    if(!region->writesAllowed || !node->writable()) return {"region_not_writable", {}};
    for(uint32_t i = 0; i < length; ++i) node->write(offset + i, bytes[i]);
    return {{}, json + ",\"written\":" + std::to_string(length) + "}"};
  }
};
