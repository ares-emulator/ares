#include "../native/execution-service.hpp"
#include "../../ares/n64/execution.hpp"
#include <cassert>
#include <iostream>

int main() {
  ExecutionService e;
  e.supported = true; e.frameKind = "synthetic-vi"; e.executor = "synthetic";
  bool paused = true;
  auto begin = [&](std::string method, std::vector<std::string> args, std::string owner = "agent") {
    return e.execute(method, args, owner, 1, paused);
  };
  assert(begin("execution.runFrames", {"-1", "1000"}).error == "invalid_request");
  assert(begin("execution.runFrames", {"1", "0"}).error == "invalid_request");
  assert(begin("execution.runFrames", {"1000001", "1000"}).error == "invalid_target");
  assert(begin("execution.runUntilFrame", {"9007199254740992", "1000"}).error == "invalid_request");
  assert(!e.active);
  // A partial starting frame finishes at the NEXT boundary, not one later.
  e.frame = 12; e.atBoundary = false;
  auto start = begin("execution.runFrames", {"1", "1000"});
  assert(start.error.empty() && e.active->target == 13 && !e.active->startBoundary);
  assert(begin("execution.runFrames", {"1", "1000"}, "competitor").error == "busy");
  assert(begin("execution.cancel", {"1"}, "competitor").error == "busy");
  e.service(paused); assert(e.active);
  e.frame = 13; e.atBoundary = true; e.service(paused);
  assert(!e.active && paused && e.finished.size() == 1);
  auto immutable = begin("execution.status", {"1"}).json;
  e.frame = 18; e.atBoundary = false;
  assert(begin("execution.status", {"1"}).json == immutable);
  assert(begin("execution.cancel", {"1"}).json == immutable);
  assert(e.finished.size() == 1); // only one final result
  assert(begin("execution.runUntilFrame", {"17", "1000"}).error == "invalid_target");
  assert(begin("execution.runFrames", {"0", "1000"}).error.empty());
  assert(!e.active && !e.atBoundary && e.frame == 18); // no fabricated boundary
  assert(begin("execution.runFrames", {"2", "1000"}).error.empty());
  e.active->deadline = ExecutionService::Clock::now(); e.service(paused);
  assert(!e.active && e.finished.back().second.find("timeout") != std::string::npos);
  begin("execution.runFrames", {"2", "1000"});
  begin("execution.release", {}, "different-generation"); assert(e.active);
  begin("execution.release", {}); assert(!e.active);
  assert(e.finished.back().second.find("disconnected") != std::string::npos);
  for(int i = 0; i < 20; ++i) begin("execution.runFrames", {"0", "1000"});
  assert(e.finished.size() == 16 && begin("execution.status", {"1"}).error == "unknown_operation");
  // A native reservation spans file-I/O gaps without a running child operation.
  auto batch = begin("execution.batchBegin", {"20000"}, "batch-fixture");
  assert(batch.error.empty() && e.reservation && paused);
  auto batchId = std::to_string(e.reservation->id);
  assert(begin("execution.runFrames", {"1", "1000"}, "foreign").error == "busy");
  assert(!e.permitted("n64.memory.write", "foreign"));
  assert(e.permitted("n64.memory.read", "foreign") && e.permitted("session.pause", "ui"));
  assert(!e.permitted("session.pause", "foreign"));
  assert(begin("execution.batchCheck", {batchId}, "foreign").error == "busy");
  assert(begin("execution.batchCheck", {batchId}, "batch-fixture").error.empty());
  assert(begin("execution.runFrames", {"1", "1000"}, "batch-fixture").error.empty());
  e.frame++; e.service(paused); assert(!e.active && e.reservation);
  e.reservation->deadline = ExecutionService::Clock::now(); e.service(paused);
  assert(!e.reservation && paused);
  assert(begin("execution.batchCheck", {batchId}, "batch-fixture").error == "batch_interrupted");
  assert(!e.permitted("n64.state.loadCommit", "batch-fixture"));
  assert(begin("execution.runFrames", {"1", "1000"}, "batch-fixture").error == "busy");
  assert(begin("execution.release", {}, "batch-fixture").error.empty());
  begin("execution.batchBegin", {"20000"}, "batch-fixture");
  begin("execution.release", {}, "foreign"); assert(e.reservation);
  begin("execution.release", {}, "batch-fixture"); assert(!e.reservation);
  auto& c = ares::Nintendo64::executionCoordinate;
  c.reset(); assert(c.frame == 0 && c.atBoundary);
  // Inactive liveness and a CPU slice leave the VI coordinate unchanged.
  c.atBoundary = false; assert(c.frame == 0);
  c.boundary(); assert(c.frame == 1 && c.boundaryLatched);
  c.reset(); assert(c.frame == 0 && !c.boundaryLatched);
  std::cout << "EXECUTION_FIXTURE_PASS: partial boundaries, immutable finals, owners, timeout, disconnect, retention\n";
}
