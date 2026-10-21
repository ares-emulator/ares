#pragma once
#include <condition_variable>
#include <cstdio>
#include <cstdint>
#include <deque>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <functional>
#include "execution-service.hpp"
#include "memory-service.hpp"

// Host-only protocol v1. Requests are bounded ASCII tokens; responses are JSON lines
// on fd 3. Execution uses extracted fork semantics, without legacy transport.
struct SessionService {
  std::string session;
  uint64_t epoch = 1;
  bool paused = false;
  ExecutionService execution;
  std::function<ControlResult(const std::string&, const std::vector<std::string>&, const std::string&)> n64Request;
  std::function<ControlResult(const std::string&, const std::vector<std::string>&, const std::string&)> videoRequest;
  std::function<ControlResult(const std::string&, const std::vector<std::string>&, const std::string&)> audioRequest;
  std::function<void(const std::string&)> onReleaseOwner;
  std::function<void(bool, const std::string&)> onResume;
  std::function<void()> onPause;
  std::function<std::string()> statusFields;
  std::function<std::string(const std::string&, const std::string&)> beforeRequest;
  FILE* output = nullptr;
  std::mutex lock;
  std::condition_variable ready, requestsReady;
  std::deque<std::string> requests, replies;
  bool closed = false;
  static constexpr size_t limit = 64;

  bool enqueue(std::string line) {
    std::lock_guard guard(lock);
    if(requests.size() >= limit) return false;
    requests.push_back(std::move(line)); requestsReady.notify_one();
    return true;
  }
  void waitForRequest(std::chrono::milliseconds duration) {
    std::unique_lock guard(lock);
    requestsReady.wait_for(guard, duration, [&] { return closed || !requests.empty(); });
  }
  void close() {
    std::lock_guard guard(lock); closed = true; ready.notify_all(); requestsReady.notify_all();
  }
  void writeReplies() {
    for(;;) {
      std::string line;
      {
        std::unique_lock guard(lock);
        ready.wait(guard, [&] { return closed || !replies.empty(); });
        if(replies.empty()) return;
        line = std::move(replies.front()); replies.pop_front();
      }
      if(std::fwrite(line.data(), 1, line.size(), output) != line.size() || std::fflush(output)) return;
    }
  }
  // Called only between root->run() invocations, including while paused.
  bool dispatch(const std::string& system, bool synthetic, const std::function<void()>& reset, MemoryService& memory) {
    execution.service(paused);
    std::deque<std::string> pending;
    { std::lock_guard guard(lock); pending.swap(requests); }
    for(auto& line : pending) {
      execution.service(paused);
      std::istringstream input(line);
      std::string prefix, version, id, identity, revision, method, extra;
      input >> prefix >> version >> id >> identity >> revision >> method;
      std::vector<std::string> arguments;
      while(input >> extra) arguments.push_back(extra);
      std::string owner = "ui";
      if(!arguments.empty() && arguments.back().starts_with("owner=")) {
        owner = arguments.back().substr(6); arguments.pop_back();
      }
      auto token = [](const std::string& value) {
        return !value.empty() && value.size() <= 64 && value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_") == std::string::npos;
      };
      bool validId = token(id);
      if(!validId) id = "invalid";
      std::string error;
      if(prefix != "control" || version != "1" || method.empty() || !validId || !token(owner)) error = "invalid_request";
      else if(identity != session) error = "stale_session";
      else if(revision != std::to_string(epoch)) error = "stale_epoch";
      else if(method.starts_with("session.") && !arguments.empty() && !(method == "session.resume" && arguments.size() == 1 && (arguments[0] == "0" || arguments[0] == "1"))) error = "invalid_request";
      else if(method != "session.status" && method != "session.pause" && method != "session.resume" && method != "session.reset" && !method.starts_with("memory.") && !method.starts_with("execution.") && !method.starts_with("n64.") && !method.starts_with("video.") && !method.starts_with("audio.")) error = "unavailable";
      if(error.empty() && method == "session.resume" && arguments == std::vector<std::string>{"1"} && !onResume) error = "unavailable";
      if(error.empty() && !execution.permitted(method, owner)) error = "busy";
      if(error.empty() && beforeRequest) error = beforeRequest(method, owner);
      ControlResult memoryResult;
      if(error.empty() && execution.active && (method == "session.resume" || method == "memory.write" ||
          (method == "session.pause" && owner != "ui" && owner != execution.active->owner))) error = "busy";
      if(error.empty() && method.starts_with("execution.")) {
        memoryResult = execution.execute(method, arguments, owner, epoch, paused);
        error = memoryResult.error;
        if(error.empty() && method == "execution.release" && onReleaseOwner) onReleaseOwner(owner);
      }
      if(error.empty() && method.starts_with("audio.")) {
        memoryResult = audioRequest ? audioRequest(method, arguments, owner) : ControlResult{"unavailable", {}};
        error = memoryResult.error;
      }
      if(error.empty() && method.starts_with("video.")) {
        memoryResult = videoRequest ? videoRequest(method, arguments, owner) : ControlResult{"unavailable", {}};
        error = memoryResult.error;
      }
      if(error.empty() && method.starts_with("n64.")) {
        memoryResult = n64Request ? n64Request(method, arguments, owner) : ControlResult{"unavailable", {}};
        error = memoryResult.error;
      }
      if(error.empty() && method.starts_with("memory.")) {
        memoryResult = memory.execute(method, arguments);
        error = memoryResult.error;
      }
      if(error.empty()) {
        if(method == "session.pause") { execution.reservation.reset(); execution.complete("paused", paused); if(onPause) onPause(); paused = true; }
        if(method == "session.resume") { execution.reservation.reset(); if(onResume) onResume(!arguments.empty() && arguments[0] == "1", owner); paused = false; }
        if(method == "session.reset") { execution.reservation.reset(); execution.complete("reset", paused); reset(); ++epoch; execution.frame = 0; execution.atBoundary = true; }
      }
      // System names originate in the compiled registry. Escape JSON defensively.
      std::string escaped;
      for(unsigned char c : system) {
        if(c == '"' || c == '\\') escaped += '\\';
        if(c >= 32) escaped += c;
      }
      std::string reply = "{\"protocol\":1,\"id\":\"" + id + "\",\"session\":\"" + session + "\",\"epoch\":" + std::to_string(epoch);
      if(!error.empty()) reply += ",\"ok\":false,\"error\":\"" + error + "\"}";
      else if(!memoryResult.json.empty()) reply += ",\"ok\":true,\"result\":" + memoryResult.json + "}";
      else reply += ",\"ok\":true,\"result\":{\"paused\":" + std::string(paused ? "true" : "false") + ",\"system\":\"" + escaped + "\",\"synthetic\":" + (synthetic ? "true" : "false") + "," + execution.coordinates() + (statusFields ? statusFields() : "") + ",\"capabilities\":[\"session.status\",\"session.pause\",\"session.resume\",\"session.reset\",\"memory.regions\",\"memory.read\",\"memory.write\",\"execution.runFrames\",\"execution.runUntilFrame\",\"execution.status\",\"execution.cancel\",\"execution.release\",\"execution.stepInstructions\",\"n64.debug.registers\",\"n64.debug.lastStop\",\"n64.debug.breakAdd\",\"n64.debug.breakRemove\",\"n64.debug.breakList\",\"n64.debug.watchAdd\",\"n64.debug.watchRemove\",\"n64.debug.watchList\",\"n64.debug.clear\",\"n64.memory.read\",\"n64.memory.write\",\"n64.memory.writeMany\",\"n64.input.set\",\"n64.input.release\",\"n64.input.state\",\"n64.input.hostMode\",\"n64.trace.start\",\"n64.trace.page\",\"n64.dma.log\",\"n64.profiler.control\",\"n64.profiler.clear\",\"n64.profiler.top\",\"n64.input.tap\",\"video.begin\",\"video.read\",\"video.release\",\"audio.capture\",\"audio.status\",\"audio.begin\",\"audio.read\",\"audio.release\",\"video.record.start\",\"video.record.status\",\"video.record.begin\",\"video.record.read\",\"video.record.release\",\"n64.state.format\",\"n64.state.layout\",\"n64.state.begin\",\"n64.state.read\",\"n64.state.release\",\"n64.state.loadBegin\",\"n64.state.loadPrepare\",\"n64.state.loadWrite\",\"n64.state.loadCommit\",\"n64.state.loadAbort\",\"execution.batchBegin\",\"execution.batchCheck\",\"n64.cpu.setRegisters\",\"n64.cpu.disassemble\",\"n64.rsp.registers\",\"n64.rdp.state\",\"n64.peripheral.registers\",\"n64.rsp.debug\",\"n64.rsp.break\",\"n64.rsp.step\",\"n64.rsp.run\",\"n64.rsp.stop\",\"n64.rdp.captureStart\",\"n64.rdp.capturePage\",\"n64.render.probe\",\"n64.dump.begin\",\"n64.dump.read\",\"n64.dump.release\",\"n64.cpu.stack\",\"n64.cpu.call\",\"n64.cpu.callRecover\",\"n64.capture.control\",\"n64.capture.rsp\",\"n64.capture.rdp\"]}}";
      std::lock_guard guard(lock);
      if(replies.size() >= limit) return false; // bounded even if peer stops reading
      replies.push_back(reply + "\n"); ready.notify_one();
    }
    return true;
  }
};
