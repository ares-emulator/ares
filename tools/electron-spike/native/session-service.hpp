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

// Host-only protocol v1. Requests are six ASCII tokens; responses are JSON lines
// on fd 3. No fork code or legacy telemetry transport is used here.
struct SessionService {
  std::string session;
  uint64_t epoch = 1;
  bool paused = false;
  FILE* output = nullptr;
  std::mutex lock;
  std::condition_variable ready;
  std::deque<std::string> requests, replies;
  bool closed = false;
  static constexpr size_t limit = 64;

  bool enqueue(std::string line) {
    std::lock_guard guard(lock);
    if(requests.size() >= limit) return false;
    requests.push_back(std::move(line));
    return true;
  }
  void close() {
    std::lock_guard guard(lock); closed = true; ready.notify_all();
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
  bool dispatch(const std::string& system, bool synthetic, const std::function<void()>& reset) {
    std::deque<std::string> pending;
    { std::lock_guard guard(lock); pending.swap(requests); }
    for(auto& line : pending) {
      std::istringstream input(line);
      std::string prefix, version, id, identity, revision, method, extra;
      input >> prefix >> version >> id >> identity >> revision >> method;
      auto token = [](const std::string& value) {
        return !value.empty() && value.size() <= 64 && value.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_") == std::string::npos;
      };
      bool validId = token(id);
      if(!validId) id = "invalid";
      std::string error;
      if(prefix != "control" || version != "1" || method.empty() || (input >> extra) || !validId) error = "invalid_request";
      else if(identity != session) error = "stale_session";
      else if(revision != std::to_string(epoch)) error = "stale_epoch";
      else if(method != "session.status" && method != "session.pause" && method != "session.resume" && method != "session.reset") error = "unavailable";
      if(error.empty()) {
        if(method == "session.pause") paused = true;
        if(method == "session.resume") paused = false;
        if(method == "session.reset") { reset(); ++epoch; }
      }
      // System names originate in the compiled registry. Escape JSON defensively.
      std::string escaped;
      for(unsigned char c : system) {
        if(c == '"' || c == '\\') escaped += '\\';
        if(c >= 32) escaped += c;
      }
      std::string reply = "{\"protocol\":1,\"id\":\"" + id + "\",\"session\":\"" + session + "\",\"epoch\":" + std::to_string(epoch);
      if(!error.empty()) reply += ",\"ok\":false,\"error\":\"" + error + "\"}";
      else reply += ",\"ok\":true,\"result\":{\"paused\":" + std::string(paused ? "true" : "false") + ",\"system\":\"" + escaped + "\",\"synthetic\":" + (synthetic ? "true" : "false") + ",\"capabilities\":[\"session.status\",\"session.pause\",\"session.resume\",\"session.reset\"]}}";
      std::lock_guard guard(lock);
      if(replies.size() >= limit) return false; // bounded even if peer stops reading
      replies.push_back(reply + "\n"); ready.notify_one();
    }
    return true;
  }
};
