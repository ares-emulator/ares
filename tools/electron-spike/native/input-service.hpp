#pragma once
#include <array>
#include <chrono>
#include <limits>
#include "memory-service.hpp"

// VI-latched controller semantics adapted from ares-64 telemetry-server.hpp and
// debug-session.cpp at 1c9b9637476a83f7b80d3db1115a0f71529820d9.
// Adds per-connection ownership; only the emulation thread touches this state.
struct InputService {
  using Clock = std::chrono::steady_clock;
  struct Port {
    std::string owner;
    uint32_t buttons = 0;
    int32_t x = 0, y = 0;
    uint64_t epoch = 0, expiresFrame = UINT64_MAX;
    Clock::time_point expiresAt = Clock::time_point::max();
    bool overridden() const { return !owner.empty(); }
    bool expired(uint64_t e, uint64_t f, Clock::time_point now) const {
      return overridden() && (epoch != e || f >= expiresFrame || now >= expiresAt);
    }
  };
  std::array<Port, 4> pending{}, latched{};
  bool available = false, neutralDuringOperations = true, operationRunning = false;
  uint64_t epoch = 0, frame = UINT64_MAX, takeovers = 0;

  void latch() { latched = pending; }
  void releaseAll() { pending = {}; latched = {}; }
  bool hasOverrides() const {
    for(size_t i = 0; i < 4; ++i) if(pending[i].overridden() || latched[i].overridden()) return true;
    return false;
  }
  void releaseOwner(const std::string& owner) {
    for(size_t i = 0; i < 4; ++i) {
      if(pending[i].owner == owner) pending[i] = {};
      if(latched[i].owner == owner) latched[i] = {};
    }
  }
  void service(uint64_t e, uint64_t f, Clock::time_point now = Clock::now(), bool force = false) {
    if(epoch && epoch != e) releaseAll();
    for(size_t i = 0; i < 4; ++i) {
      if(pending[i].expired(e, f, now)) { pending[i] = {}; latched[i] = {}; }
      if(latched[i].expired(e, f, now)) latched[i] = {};
    }
    if(force || epoch != e || frame != f) latch();
    epoch = e; frame = f;
  }
  void start(uint64_t e, uint64_t f) { operationRunning = true; service(e, f, Clock::now(), true); }
  void finish(uint64_t e, uint64_t f) { operationRunning = false; service(e, f, Clock::now(), true); }
  bool takeover() {
    if(!hasOverrides() && !operationRunning) return false;
    releaseAll(); operationRunning = false; ++takeovers; return true;
  }
  static bool axis(const std::string& text, int32_t& value) {
    auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    return !text.empty() && error == std::errc{} && end == text.data() + text.size() && value >= -32767 && value <= 32767;
  }
  std::string state() const {
    auto encode = [](const Port& p) {
      return "{\"overridden\":" + std::string(p.overridden() ? "true" : "false") + ",\"owner\":" + (p.overridden() ? controlQuote(p.owner) : "null")
        + ",\"buttons\":" + std::to_string(p.buttons) + ",\"x\":" + std::to_string(p.x) + ",\"y\":" + std::to_string(p.y)
        + ",\"expires_frame\":" + (p.expiresFrame == UINT64_MAX ? "null" : std::to_string(p.expiresFrame)) + "}";
    };
    std::string out = "{\"ports\":[";
    for(size_t i = 0; i < 4; ++i) {
      if(i) out += ',';
      out += "{\"port\":" + std::to_string(i + 1) + ",\"pending\":" + encode(pending[i]) + ",\"latched\":" + encode(latched[i]) + "}";
    }
    return out + "],\"epoch\":" + std::to_string(epoch) + ",\"frame\":" + std::to_string(frame == UINT64_MAX ? 0 : frame)
      + ",\"neutral_during_operations\":" + (neutralDuringOperations ? "true" : "false") + ",\"operation_running\":" + (operationRunning ? "true" : "false")
      + ",\"human_takeovers\":" + std::to_string(takeovers) + "}";
  }
  ControlResult execute(const std::string& method, const std::vector<std::string>& args, const std::string& owner,
                        uint64_t e, uint64_t f, const std::string& executionOwner, Clock::time_point now = Clock::now()) {
    if(!available) return {"unavailable", {}};
    service(e, f, now);
    if(method == "n64.input.state") return args.empty() ? ControlResult{{}, state()} : ControlResult{"invalid_request", {}};
    if(method != "n64.input.release" && !executionOwner.empty() && owner != "ui" && executionOwner != owner) return {"busy", {}};
    if(method == "n64.input.hostMode") {
      if(args.size() != 1 || (args[0] != "0" && args[0] != "1")) return {"invalid_request", {}};
      neutralDuringOperations = args[0] == "1";
      return {{}, state()};
    }
    if(method == "n64.input.release") {
      uint32_t port;
      if(args.size() != 1 || !MemoryService::number(args[0], port) || port > 4) return {"invalid_request", {}};
      // A foreign caller cannot revoke another connection's port. UI may revoke all.
      if(port && owner != "ui" && ((pending[port - 1].overridden() && pending[port - 1].owner != owner) ||
          (latched[port - 1].overridden() && latched[port - 1].owner != owner))) return {"busy", {}};
      for(size_t i = 0; i < 4; ++i) if(!port || i == port - 1) {
        if(owner == "ui" || pending[i].owner == owner) pending[i] = {};
        if(owner == "ui" || latched[i].owner == owner) latched[i] = {};
      }
      return {{}, state()};
    }
    if(method != "n64.input.set") return {"unavailable", {}};
    uint32_t port, buttons, frames, milliseconds; int32_t x, y;
    if(args.size() != 6 || !MemoryService::number(args[0], port) || port < 1 || port > 4 || !MemoryService::number(args[1], buttons) || buttons > 0x3fff ||
       !axis(args[2], x) || !axis(args[3], y) || !MemoryService::number(args[4], frames) || frames > 100000 ||
       !MemoryService::number(args[5], milliseconds) || milliseconds > 3600000) return {"invalid_request", {}};
    auto& p = pending[port - 1]; auto& l = latched[port - 1];
    if(owner != "ui" && ((p.overridden() && p.owner != owner) || (l.overridden() && l.owner != owner))) return {"busy", {}};
    if(frames && f > 9007199254740991ull - frames) return {"invalid_request", {}};
    p = {owner, buttons, x, y, e, frames ? f + frames : UINT64_MAX, milliseconds ? now + std::chrono::milliseconds(milliseconds) : Clock::time_point::max()};
    return {{}, state()};
  }
  bool apply(ares::Node::Input::Input node, uint64_t e, uint64_t f) {
    if(!available || node->cast<ares::Node::Input::Rumble>()) return false;
    service(e, f);
    auto device = ares::Node::parent(node); if(!device) return false;
    auto port = ares::Node::parent(device); if(!port) return false;
    std::string name = port->name().data();
    if(name.size() != 17 || name.substr(0, 16) != "Controller Port " || name[16] < '1' || name[16] > '4') return false;
    auto& p = latched[name[16] - '1'];
    bool neutral = !p.overridden() && operationRunning && neutralDuringOperations;
    if(!neutral && !p.overridden()) return false;
    static constexpr const char* buttons[] = {"A", "B", "Z", "Start", "Up", "Down", "Left", "Right", "C-Up", "C-Down", "C-Left", "C-Right", "L", "R"};
    if(auto button = node->cast<ares::Node::Input::Button>()) {
      uint32_t value = 0;
      if(!neutral) for(unsigned i = 0; i < 14; ++i) if(node->name() == buttons[i]) value = (p.buttons >> i) & 1;
      button->setValue(value); return true;
    }
    if(auto axis = node->cast<ares::Node::Input::Axis>()) {
      axis->setValue(neutral ? 0 : node->name() == "X-Axis" ? p.x : node->name() == "Y-Axis" ? p.y : 0); return true;
    }
    if(auto trigger = node->cast<ares::Node::Input::Trigger>()) { trigger->setValue(0); return true; }
    return false;
  }
};
