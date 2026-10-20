// Core assertions adapted from ares-64 tests/mcp/live_fixture.py at
// 1c9b9637476a83f7b80d3db1115a0f71529820d9. No ROM or live user machine.
#include <n64/n64.hpp>
#include "../native/n64-service.hpp"
#include "../native/input-service.hpp"
#include "../native/trace-service.hpp"
#include "../native/dma-service.hpp"
#include "../native/profiler-service.hpp"
#include "../native/tap-service.hpp"
#include "../native/frame-service.hpp"
#include "../native/audio-service.hpp"
#include "../native/recording-service.hpp"
#include <cstdio>
#include <cstdlib>
using namespace ares::Nintendo64;
using Debug = CPU::DebugControl;
static void check(bool ok, const char* name) {
  if(!ok) { std::fprintf(stderr, "FAIL %s\n", name); std::exit(1); }
}
static constexpr uint64_t base = 0xffffffff80001000ull;
static constexpr uint64_t data = 0xffffffff80002000ull;
static void pc(uint64_t address) { cpu.pipeline.setPc(address); }
static void word(unsigned offset, uint32_t value) {
  rdram.ram.write<Word>(0x1000 + offset, value, RBusDevice::ARES_DEBUGGER);
}
static uint32_t mem(unsigned op, unsigned rt, unsigned offset) {
  return op << 26 | 8 << 21 | rt << 16 | offset;
}
static void steps(unsigned count) {
  auto& d = cpu.debugControl;
  d.resume(); d.requestStepInstructions(count);
  for(unsigned i = 0; i <= count && !d.halted; ++i) cpu.instruction();
  check(d.halted, "bounded instruction stop");
}
int main() {
  {
    RecordingService video;
    std::vector<uint8_t> rgba{255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255};
    check(video.execute("video.record.start", {"2", "1048576", "60", "1000", "0"}, "one", 1, 0).error.empty(), "record start");
    video.capture(rgba.data(), 2, 2, 1, 1); video.capture(rgba.data(), 2, 2, 1, 2); video.capture(rgba.data(), 2, 2, 1, 3);
    check(video.frames.size() == 2 && video.reason == "frames_limit", "record bounded frame limit");
    check(video.frames[0] == std::vector<uint8_t>({255,0,0,255,255,255,0,0, 0,0,255,0,255,0,0,0}), "record bottom-up BGR row padding");
    check(video.execute("video.record.begin", {}, "two", 1, 2).error == "busy", "record foreign export");
    video.execute("video.record.begin", {}, "one", 1, 2); auto id = std::to_string(video.pin);
    check(video.execute("video.record.start", {"2", "1048576", "60", "1000", "1"}, "one", 1, 2).error == "busy", "record pin prevents discard");
    check(video.execute("video.record.read", {id, "31", "2"}, "one", 1, 2).error == "out_of_bounds", "record read bounds");
    video.execute("video.record.release", {id, "0"}, "one", 1, 2);
    check(video.available && video.frames.size() == 2, "failed video export retains frames");
    video.execute("video.record.start", {"2", "1048576", "60", "1000", "1"}, "one", 1, 2);
    video.capture(rgba.data(), 2, 2, 2, 3); check(video.reason == "epoch_changed", "record old epoch rejected");
    video.execute("video.record.start", {"2", "1048576", "60", "1000", "1"}, "one", 1, 2);
    video.capture(rgba.data(), 2, 2, 1, 3); video.capture(rgba.data(), 3, 2, 1, 4);
    check(video.reason == "dimensions_changed" && video.frames.size() == 1, "record dimension stop");
    video.releaseOwner("one"); check(!video.available && video.frames.empty(), "record disconnect cleanup");
    video.execute("video.record.start", {"2", "1048576", "60", "1000", "0"}, "one", 1, 0);
    video.deadline = RecordingService::Clock::now(); video.execute("video.record.status", {}, "two", 1, 0);
    check(video.reason == "seconds_limit", "record wall deadline");
    video.reset(); video.execute("video.record.start", {"10", "1048576", "60", "1000", "0"}, "one", 1, 0);
    std::vector<uint8_t> large(512 * 512 * 4);
    video.capture(large.data(), 512, 512, 1, 1); video.capture(large.data(), 512, 512, 1, 2);
    check(video.reason == "bytes_limit" && video.frames.size() == 1, "record byte bound");
  }

  {
    AudioService audio;
    check(audio.execute("audio.capture", {"2"}, "one", false).error == "unavailable", "audio unavailable without streams");
    check(audio.execute("audio.capture", {"2"}, "one", true).error.empty(), "audio capture start");
    audio.push(2, -2);
    check(audio.samples == std::vector<uint8_t>({0xff, 0x7f, 0x01, 0x80}), "audio stereo PCM clamp/endian");
    auto pin = audio.execute("audio.begin", {}, "one", true);
    check(pin.error.empty() && audio.pin->bytes.size() == 4, "partial audio pin");
    auto id = std::to_string(audio.pin->id);
    audio.push(0.5, -0.5); audio.push(1, 1);
    check(audio.samples.size() == 8 && audio.pin->bytes.size() == 4, "audio cap freezes and pin immutable");
    check(audio.execute("audio.capture", {"0"}, "one", true).error == "busy", "audio pin prevents capture replacement");
    check(audio.execute("audio.read", {id, "0", "4"}, "two", true).error == "busy", "audio foreign read rejected");
    check(audio.execute("audio.capture", {"0"}, "two", true).error == "busy", "audio owner retained");
    check(audio.execute("audio.release", {id, "0"}, "one", true).error.empty() && audio.active, "failed save retains capture");
    audio.execute("audio.begin", {}, "one", true); id = std::to_string(audio.pin->id);
    check(audio.execute("audio.read", {id, "7", "2"}, "one", true).error == "out_of_bounds", "audio read bounds");
    audio.execute("audio.release", {id, "1"}, "one", true);
    check(!audio.active && audio.samples.empty(), "saved capture consumed");
    audio.execute("audio.capture", {"0"}, "one", true); audio.push(1, 1);
    check(audio.samples.empty() && audio.active, "zero audio cap is empty completed capture");
    audio.releaseOwner("two"); check(audio.active, "foreign disconnect preserves audio");
    audio.releaseOwner("one"); check(!audio.active && audio.cap == 0, "owner disconnect revokes audio");
    audio.execute("audio.capture", {"192001"}, "one", true);
    check(!audio.active, "oversized audio capture rejected");
  }

  std::vector<uint8_t> hidden(2 * 1024 * 1024);
  rdram.hidden.data = hidden.data();
  rdram.ram.allocate(4 * 1024 * 1024); rdram.mapIdentity = 1; rdram.ram.fill();
  auto root = std::make_shared<ares::Core::Object>("CPU fixture");
  // Task 6 input foundation: real ares controller nodes, deterministic clocks.
  InputService input; input.available = true;
  const auto now = InputService::Clock::now();
  auto set = [&](const char* port, const char* mask, const char* frames, const char* milliseconds, const char* owner) {
    return input.execute("n64.input.set", {port, mask, "1234", "-2345", frames, milliseconds}, owner, 1, input.frame, "", now);
  };
  input.service(1, 10, now);
  check(set("1", "1", "2", "0", "owner-a").error.empty(), "input port acquired");
  check(!input.latched[0].overridden(), "new input waits for VI latch");
  input.service(1, 10, now);
  check(!input.latched[0].overridden(), "same VI cannot relatch pending input");
  input.service(1, 11, now);
  check(input.latched[0].buttons == 1 && input.latched[0].x == 1234, "next VI latches held input");
  check(set("1", "2", "0", "0", "owner-b").error == "busy", "foreign port acquisition rejected");
  check(set("2", "2", "0", "0", "owner-b").error.empty(), "independent port ownership");
  input.releaseOwner("owner-a");
  check(!input.pending[0].overridden() && !input.latched[0].overridden() && input.pending[1].owner == "owner-b", "disconnect revokes only owner ports immediately");
  input.start(1, 11);
  check(input.latched[1].buttons == 2 && input.operationRunning, "operation start latches pending input");
  auto port = root->append<ares::Node::Port>("Controller Port 2");
  auto pad = port->append<ares::Node::Peripheral>("Gamepad");
  auto button = pad->append<ares::Node::Input::Button>("B");
  auto axis = pad->append<ares::Node::Input::Axis>("X-Axis");
  auto motor = pad->append<ares::Node::Input::Rumble>("Rumble");
  check(input.apply(button, 1, 11) && button->value() == 1, "injection reaches real controller button");
  check(input.apply(axis, 1, 11) && axis->value() == 1234, "injection reaches real controller axis");
  check(!input.apply(motor, 1, 11), "rumble remains host output");
  input.releaseOwner("owner-b"); button->setValue(1);
  check(input.apply(button, 1, 11) && button->value() == 0, "unoverridden host input neutral during operation");
  input.neutralDuringOperations = false;
  check(!input.apply(button, 1, 11), "opt-in host mixing passes through");
  input.finish(1, 11);
  check(set("3", "4", "1", "0", "owner-c").error.empty(), "frame lease acquired");
  input.service(1, 11, now, true);
  check(input.pending[2].overridden(), "frame lease holds until native target VI");
  input.service(1, 12, now);
  check(!input.pending[2].overridden(), "frame lease expires against native VI coordinate");
  check(set("4", "8", "0", "5", "owner-d").error.empty(), "wall-time lease acquired");
  input.service(1, 12, now, true);
  input.service(1, 12, now + std::chrono::milliseconds(6));
  check(!input.pending[3].overridden() && !input.latched[3].overridden(), "wall lease expires while paused at same frame");
  check(set("1", "1", "0", "0", "owner-a").error.empty(), "unbounded hold acquired");
  input.start(1, 12);
  check(input.takeover() && !input.hasOverrides() && !input.operationRunning && input.takeovers == 1, "human takeover revokes held and operation input");
  check(!input.takeover(), "neutral duplicate input does not create takeover");
  check(set("1", "1", "0", "0", "owner-a").error.empty(), "epoch fixture acquired");
  input.service(2, 0, now);
  check(!input.hasOverrides(), "epoch replacement revokes every hold");
  cpu.load(root);
  cpu.power(false);
  cpu.recompiler.enabled = true;
  cpu.scc.configuration.bigEndian = 1; cpu.context.setMode();
  auto& d = cpu.debugControl;
  nall::string error; uint32_t physical = 0;
  for(uint64_t address : {0x2000ull, 0x80002000ull, 0xa0002000ull, data, 0xffffffffa0002000ull})
    { check(d.normalizeAddress(address, 4, physical, error) && physical == 0x2000, "physical/KSEG aliases"); }
  for(uint64_t address : {0xc0002000ull, 0x100000000ull, 0xffffffffc0002000ull, 0x400000ull, ~0ull})
    check(!d.normalizeAddress(address, 4, physical, error), "unsupported addresses rejected");
  check(!d.normalizeAddress(0x3fffff, 2, physical, error), "range cannot wrap installed RAM");
  check(!d.normalizeAddress(0, 0, physical, error), "empty range rejected");
  cpu.ipu.r[8].u64 = data; cpu.ipu.r[9].u64 = 0x11223344;
  word(0, mem(0x2b, 9, 0));       // sw t1,0(t0)
  word(4, mem(0x23, 10, 0));      // lw t2,0(t0)
  word(8, mem(0x22, 11, 1));      // lwl t3,1(t0)
  word(12, mem(0x26, 11, 4));     // lwr t3,4(t0)
  word(16, 0x10000002);           // beq zero,zero,+2
  word(20, 0x240d0001);           // addiu t5,zero,1 (delay)
  word(24, 0x240e0002);           // skipped
  word(28, 0);                   // nop
  pc(base);
  auto watch = d.addWatchpoint(0xa0002000, 4, Debug::AccessWrite, {}, error);
  check(watch != 0, "write watch added via KSEG1");
  steps(20);
  check(d.stop.reason == Debug::StopReason::Watchpoint, "watchpoint wins over remaining steps");
  check(d.stop.access.pc == base && d.stop.snapshot.pc == base + 4, "write hit is post-instruction");
  check(d.stop.access.value == 0x11223344 && d.stop.access.size == 4 && d.stop.access.hasOld && d.stop.access.oldValue == 0, "write value and old bytes");
  auto frozen = d.stop;
  d.removeWatchpoint(watch);
  steps(1);
  check(cpu.ipu.r[10].u64 == 0x11223344, "load uses dirty CPU cache");
  check(frozen.snapshot.pc == base + 4 && frozen.snapshot.gpr[10] == 0, "prior snapshot immutable");
  auto excluded = d.addWatchpoint(data, 1, Debug::AccessRead, {}, error);
  auto included = d.addWatchpoint(data + 1, 1, Debug::AccessRead, {}, error);
  steps(1);
  check(d.stop.access.watchId == included && d.stop.access.size == 3 && d.stop.access.paddr == 0x2001 && d.stop.access.value == 0x223344, "LWL semantic bytes only");
  check(d.watchpoints[0].hits == 0, "LWL excludes aligned container prefix");
  d.removeWatchpoint(excluded); d.removeWatchpoint(included);
  included = d.addWatchpoint(data + 4, 1, Debug::AccessRead, {}, error);
  excluded = d.addWatchpoint(data + 5, 1, Debug::AccessRead, {}, error);
  steps(1);
  check(d.stop.access.watchId == included && d.stop.access.size == 1 && d.watchpoints[1].hits == 0, "LWR semantic bytes only");
  d.removeWatchpoint(excluded); d.removeWatchpoint(included);
  steps(1);
  check(d.stop.snapshot.pc == base + 20 && d.stop.snapshot.delaySlot, "branch stops with pending delay slot");
  steps(1);
  check(d.stop.snapshot.pc == base + 28 && !d.stop.snapshot.delaySlot && cpu.ipu.r[13].u64 == 1 && cpu.ipu.r[14].u64 == 0, "delay slot is one instruction");
  auto breakpoint = d.addBreakpoint(0xa000101c, {}, error);
  d.resume(); cpu.instruction();
  check(d.halted && d.stop.reason == Debug::StopReason::Breakpoint && d.stop.breakpointId == breakpoint && d.stop.snapshot.pc == base + 28, "alias breakpoint stops before fetch");
  steps(1);
  check(d.stop.reason == Debug::StopReason::InstructionsDone && d.stop.snapshot.pc == base + 32, "resume suppresses same breakpoint once");
  d.removeBreakpoint(breakpoint); d.resume();
  check(!d.active && !d.gate && !d.watchActive && cpu.recompiler.enabled, "debug disable restores executor eligibility");
  // Cache helpers must neither fill on a miss nor write back a dirty resident line.
  auto& line = cpu.dcache.line(data);
  check(line.hit(0x2000) && line.dirty, "fixture has resident dirty cache line");
  check(rdram.ram.read<Word>(0x2000, RBusDevice::ARES_DEBUGGER) == 0, "CPU store stays out of physical RDRAM");
  check(cpu.dcache.readDebug<Byte>(data, 0x2000) == 0x11, "CPU view sees dirty bytes");
  cpu.dcache.writeDebug<Byte>(data, 0x2000, 0x55);
  check(cpu.dcache.readDebug<Byte>(data, 0x2000) == 0x55 && rdram.ram.read<Byte>(0x2000, RBusDevice::ARES_DEBUGGER) == 0, "CPU-view write updates cache only");
  auto& miss = cpu.dcache.line(data + 16);
  check(!miss.valid(), "miss starts absent");
  cpu.dcache.writeDebug<Byte>(data + 16, 0x2010, 0xab);
  check(!miss.valid() && cpu.dcache.readDebug<Byte>(data + 16, 0x2010) == 0xab, "debug miss writes RDRAM without allocation");
  check((d.snapshot().cop0Valid & 2) == 0, "snapshot omits side-effectful Random register");
  auto cpuRead = N64Service::memory("n64.memory.read", {"0x80002000", "cpu", "1"}, false);
  check(cpuRead.error.empty() && cpuRead.json.find("\"hex\":\"55\"") != std::string::npos, "strict service CPU view");
  auto uncached = N64Service::memory("n64.memory.read", {"0xffffffffa0002000", "cpu", "1"}, false);
  check(uncached.error.empty() && uncached.json.find("\"hex\":\"00\"") != std::string::npos, "strict service KSEG1 bypass");
  auto patch = N64Service::memory("n64.memory.write", {"0x2000", "rdram", "77"}, false);
  check(patch.error.empty() && patch.json.find("\"dcache_dirty_lines_overlapping\":1") != std::string::npos, "physical patch reports dirty-cache hazard");
  check(rdram.ram.read<Byte>(0x2000, RBusDevice::ARES_DEBUGGER) == 0x77 && cpu.dcache.readDebug<Byte>(data, 0x2000) == 0x55, "physical patch preserves resident cache");
  check(N64Service::memory("n64.memory.write", {"0x3fffff", "rdram", "1122"}, false).error == "out_of_bounds", "service prevalidates whole range");
  check(N64Service::memory("n64.memory.write", {"0x2000", "cpu", "99zz"}, false).error == "invalid_request", "service prevalidates all hex bytes");
  check(cpu.dcache.readDebug<Byte>(data, 0x2000) == 0x55, "invalid write makes no partial change");
  // Exercise the actual JIT dispatch again after removing the last debug hook.
  word(0x80, 0x1000ffff); word(0x84, 0);
  cpu.icache.power(false); pc(base + 0x80);
  cpu.instruction();
  check(cpu.recompiler.sections[cpu.recompiler.sectionIndex(0x1080)] != nullptr, "ordinary dispatch actually creates a recompiler block again");
  // PC histogram buckets count actual instructions and bound retained records.
  executionCoordinate.frame = 10; pc(base + 0x80);
  d.beginTrace(true, false, 10, 12, base, base + 0x200, 1);
  steps(4); d.endTrace(); d.resume();
  check(d.trace.pcRecords.size() == 1 && d.trace.pcRecords[0].frame == 10 && d.trace.pcRecords[0].count == 2, "PC histogram frame bucket and count");
  check(d.trace.truncated && d.trace.droppedRecords == 1 && d.trace.totalInstructions == 4 && d.trace.instructionsObserved == 4, "trace record cap preserves counters");
  check(d.trace.counters.capacity() == 0 && d.trace.vaddrs.capacity() == 0 && !d.active, "trace disable releases histogram storage and interpreter gate");
  pc(base + 0x80);
  d.beginTrace(true, false, 10, 11, base + 0x80, base + 0x84, 4);
  steps(2); d.endTrace(); d.resume();
  check(d.trace.pcRecords.size() == 1 && d.trace.totalInstructions == 1 && d.trace.instructionsObserved == 2, "half-open executing-PC filter");
  d.beginTrace(true, false, 10, 11, 0, ~0ull, 1);
  cpu.debugTraceInstruction(0xffffffffbfc00000ull, 0x1fc00000, 0);
  cpu.debugTraceInstruction(0xffffffffbfc00004ull, 0x1fc00004, 0);
  check(d.trace.other.size() == 1 && d.trace.overflowObservations == 1 && d.trace.truncated, "non-RDRAM histogram map is bounded before VI flush");
  d.endTrace(); d.resume();
  d.beginTrace(true, false, 10, 11, 0, ~0ull, 1);
  auto index = 0x1080u >> 2;
  d.trace.counters[index] = ~0u - 1; d.trace.touched.push_back(index); d.trace.vaddrs[index] = base + 0x80;
  cpu.debugTraceInstruction(base + 0x80, 0x1080, 0);
  cpu.debugTraceInstruction(base + 0x80, 0x1080, 0);
  check(d.trace.counters[index] == ~0u && d.trace.touched.size() == 1 && d.trace.overflowObservations == 1, "histogram counters saturate with explicit overflow");
  d.endTrace(); d.resume();
  // JALR t8,t8 must capture the old target before t8 receives the link address.
  word(0x100, (24u << 21) | (24u << 11) | 9); word(0x104, 0);
  word(0x180, (24u << 21) | 8); word(0x184, 0);
  cpu.icache.power(false); pc(base + 0x100); cpu.ipu.r[24].u64 = base + 0x180;
  d.beginTrace(false, true, 10, 11, base + 0x100, base + 0x104, 4);
  d.trace.registerMask = (1u << 24) | (1u << 29); d.trace.captureF12 = true;
  d.trace.targetLow = base + 0x180; d.trace.targetHigh = base + 0x184;
  steps(4); d.endTrace(); d.resume();
  check(d.trace.callRecords.size() == 1 && d.trace.callsObserved == 1, "linked-call trace excludes plain JR");
  auto call = d.trace.callRecords[0];
  check(call.pc == base + 0x100 && call.target == base + 0x180 && call.linkRegister == 24 && call.linkValue == base + 0x108, "JALR target uses pre-instruction source");
  check(call.registers[24] == base + 0x180 && cpu.ipu.r[24].u64 == base + 0x108, "call argument snapshot precedes link write");
  TraceService view; view.available = true; view.calls = true; view.operationId = 99; view.epoch = 1;
  view.startFrame = 10; view.endFrame = 11; view.maxRecords = 4; view.registerMask = (1u << 24) | (1u << 29); view.f12 = true;
  view.finish("instructions_done");
  auto callPage = view.page({"99", "0", "64"}, 1);
  check(callPage.error.empty(), "real linked-call record serializes through native trace service");
  std::printf("TRACE_CALL_PAGE %s\n", callPage.json.c_str());

  pc(base + 0x100); cpu.ipu.r[24].u64 = base + 0x180;
  d.beginTrace(false, true, 10, 11, base + 0x100, base + 0x104, 4);
  d.trace.targetLow = base + 0x1a0; d.trace.targetHigh = base + 0x1a4;
  steps(1); d.endTrace(); d.resume();
  check(d.trace.callRecords.empty() && d.trace.callsObserved == 1 && d.trace.excludedTarget == 1, "target filter reports excluded calls");
  check(!d.active, "call-trace disable restores normal executor eligibility");
  auto observed = d.trace.instructionsObserved;
  cpu.recompiler.reset(); pc(base + 0x80);
  for(unsigned n = 0; n < 3; ++n) cpu.instruction();
  auto section = cpu.recompiler.sections[cpu.recompiler.sectionIndex(0x1080)];
  check(section && section->blocks[cpu.recompiler.blockIndex(0x1080)] && !d.active && d.trace.instructionsObserved == observed, "trace disable returns to real JIT blocks without capturing");
  // An actual SW to PI_WR_LEN starts the transfer; metadata must match bytes.
  pi.load(root); pi.power(false); cartridge.rom.allocate(16, 0);
  for(unsigned n = 0; n < 16; ++n) cartridge.rom.data[n ^ 3] = 0x80 + n;
  pi.attach(cartridge.romDevice, 0);
  dmaProvenance.enabled = true; dmaProvenance.reset(7);
  word(0x200, 0x3c08a460); word(0x204, 0x24093000); word(0x208, mem(0x2b, 9, 0));
  word(0x20c, 0x3c091000); word(0x210, mem(0x2b, 9, 4)); word(0x214, 0x24090007); word(0x218, mem(0x2b, 9, 12));
  auto dmaWatch = d.addWatchpoint(0x3000, 8, Debug::AccessWrite, {}, error);
  cpu.icache.power(false); pc(base + 0x200); steps(7);
  check(d.stop.reason == Debug::StopReason::InstructionsDone && d.watchpoints[0].hits == 0, "PI DMA is provenance, not a CPU data-watch hit");
  d.removeWatchpoint(dmaWatch);
  check(dmaProvenance.records.size() == 1 && dmaProvenance.current == nullptr, "CPU-initiated PI DMA captured once");
  auto transfer = dmaProvenance.records[0];
  check(transfer.pc == base + 0x218 && transfer.epoch == 7 && transfer.frame == 10 && transfer.fromRom && transfer.romOffset == 0, "DMA initiating PC, epoch, frame and ROM offset");
  check(transfer.requestedLength == 8 && transfer.bytesWritten == 8 && transfer.bytesAttempted == 8 && transfer.ranges.size() == 1 && transfer.ranges[0].address == 0x3000 && transfer.ranges[0].length == 8, "DMA metadata matches aligned write range");
  for(unsigned n = 0; n < 8; ++n) check(rdram.ram.read<Byte>(0x3000 + n, RBusDevice::ARES_DEBUGGER) == 0x80 + n, "real PI bytes match original ROM pattern");
  pi.power(false); pi.io.pbusAddress = 0x10000000; pi.io.dramAddress = 0x3012; pi.io.writeLength = 7; pi.io.originPc = base + 0x218;
  pi.dmaWrite();
  auto partial = dmaProvenance.records.back();
  check(partial.requestedLength == 8 && partial.bytesWritten == 6 && partial.ranges[0].address == 0x3012 && partial.ranges[0].length == 6, "PI misalignment records actual short first-block writes");
  for(unsigned n = 0; n < 6; ++n) check(rdram.ram.read<Byte>(0x3012 + n, RBusDevice::ARES_DEBUGGER) == 0x80 + n, "short PI range agrees with actual RAM");
  check(rdram.ram.read<Byte>(0x3018, RBusDevice::ARES_DEBUGGER) == 0, "PI provenance does not extend through untouched bytes");
  DmaService dmaView; dmaView.available = true;
  auto dmaPage = dmaView.execute({"0", "16", "1024", "2359296"});
  check(dmaPage.error.empty(), "DMA log serializes actual transfers");
  std::printf("DMA_PAGE %s\n", dmaPage.json.c_str());
  // Stress the history boundary with real transfer loops (guest scheduling is not advanced).
  dmaProvenance.reset(8);
  for(unsigned n = 0; n < DmaProvenance::capacity + 1; ++n) {
    pi.power(false); pi.io.pbusAddress = 0x10000000; pi.io.dramAddress = 0x3000; pi.io.writeLength = 7; pi.io.originPc = base + 0x218; pi.dmaWrite();
  }
  check(dmaProvenance.records.size() == DmaProvenance::capacity && dmaProvenance.dropped == 1, "real DMA history is a bounded overwrite ring");
  auto ringPage = dmaView.execute({"0", "16", "1024", "2359296"});
  check(ringPage.error.empty(), "overwritten DMA history serializes");
  std::printf("DMA_RING_PAGE %s\n", ringPage.json.c_str());
  DmaProvenance bounded; bounded.enabled = true; bounded.reset(1);
  bounded.begin(0x10000000, 0, 1000, base, 16, 4 * 1024 * 1024, true);
  for(unsigned n = 0; n < DmaProvenance::maxRanges + 1; ++n) bounded.wrote(n * 16, 1);
  check(bounded.current->ranges.size() == DmaProvenance::maxRanges && bounded.current->rangesTruncated && bounded.current->bytesWritten == DmaProvenance::maxRanges + 1, "DMA range truncation is explicit and byte totals survive");
  bounded.begin(0x10000000, 0, 8, base, 16, 4 * 1024 * 1024, false); bounded.wrote(0, 8);
  check(!bounded.current->writesConfirmed && bounded.current->bytesAttempted == 8, "nonidentity RDRAM writes remain unconfirmed");
  bounded.reset(2); check(bounded.records.empty() && bounded.current == nullptr && bounded.epoch == 2, "epoch reset revokes old provenance");
  // Profiler: original MIPS call/return and exception bodies, not a commercial ROM.
  auto& prof = cpu.profiler;
  word(0x300, 0x0c0004d0); word(0x304, 0);  // jal 80001340; delay
  word(0x308, 0x1000ffff); word(0x30c, 0);  // returned caller loops
  word(0x340, 0x24020001); word(0x344, 0x03e00008); word(0x348, 0);
  cpu.ipu.r[29].u64 = 0xffffffff80003000ull;
  cpu.icache.power(false); pc(base + 0x300); prof.setEnabled(true); steps(6);
  check(prof.stats.size() == 1 && prof.stats.at(0x80001340).callCount == 1, "profiler accounts one real call on return");
  auto functionCost = prof.stats.at(0x80001340);
  check(functionCost.inclCycles == 106 && functionCost.exclCycles == 106 && functionCost.exclBytesIn == 32, "profiler exact master ticks and RDRAM line bytes");
  auto timeBeforeCount = prof.now(); cpu.flushCount();
  check(prof.now() == timeBeforeCount, "profiler COUNT flush does not double count pending time");
  ProfilerService profView; profView.available = true; profView.owner = "fixture";
  auto top = profView.execute("n64.profiler.top", {"continuous", "32"}, "observer", false);
  check(top.error.empty(), "profiler top safe-point serialization"); std::printf("PROFILER_TOP %s\n", top.json.c_str());
  check(profView.execute("n64.profiler.control", {"0"}, "foreign", false).error == "busy", "profiler capture owner blocks foreign toggles");
  prof.onFrame(); check(prof.frameStats.at(0x80001340).callCount == 1 && prof.frameCount == 1, "profiler swap window commits completed calls");
  prof.onFrame(); check(prof.frameStats.empty() && prof.frameCount == 2, "profiler frame window does not reuse earlier data");
  prof.clearStats(); check(prof.stats.empty() && prof.frameCount == 0, "profiler clear rebases statistics");
  // Synchronous syscall cost nests beneath the interrupted function, ERET unwinds it.
  word(0x340, 0x0000000c);
  rdram.ram.write<Word>(0x180, 0x401a7000, RBusDevice::ARES_DEBUGGER); // mfc0 k0,EPC
  rdram.ram.write<Word>(0x184, 0x275a0004, RBusDevice::ARES_DEBUGGER);
  rdram.ram.write<Word>(0x188, 0x409a7000, RBusDevice::ARES_DEBUGGER); // mtc0 k0,EPC
  rdram.ram.write<Word>(0x18c, 0x42000018, RBusDevice::ARES_DEBUGGER); // eret
  cpu.scc.status.vectorLocation = 0; cpu.scc.status.exceptionLevel = 0; cpu.scc.status.errorLevel = 0; cpu.context.setMode();
  cpu.icache.power(false); pc(base + 0x300); steps(10);
  check(prof.stats.count(0xe0000048) && prof.excActive == 0 && prof.callStack.empty(), "profiler syscall and ERET balance synthetic exception frame");
  auto handler = prof.stats.at(0xe0000048); auto interrupted = prof.stats.at(0x80001340);
  check(handler.inclCycles > 0 && interrupted.inclCycles == interrupted.exclCycles + handler.inclCycles, "exception time excluded from interrupted function exclusive cost");
  check(interrupted.exclBytesIn == 32 && handler.exclBytesIn == 32, "exception bus bytes attributed separately");
  // Profiling JIT blocks uses the prologue hook, without debug interpreter forcing.
  prof.setEnabled(false); prof.clearStats(); word(0x340, 0x24020001);
  d.resume(); d.updateActive(); cpu.icache.power(false); pc(base + 0x300);
  prof.setEnabled(true); check(cpu.recompiler.callInstructionPrologue && !d.active, "profiler enables JIT prologue without forcing interpreter");
  cpu.jitClockTarget = cpu.clock + 100000; // This fixture drives instruction(), without the host scheduler.
  for(unsigned n = 0; n < 10 && prof.stats.empty(); ++n) cpu.instruction();
  check(prof.stats.count(0x80001340) && prof.stats.at(0x80001340).callCount == 1 && prof.stats.at(0x80001340).exclBytesIn == 32, "real JIT calls and inline icache miss bytes are profiled");
  cpu.debugger.tracer.instruction->setEnabled(true);
  profView.releaseOwner("old-generation"); check(prof.enabled, "stale profiler owner cannot disable new capture");
  profView.releaseOwner("fixture"); check(!prof.enabled && cpu.recompiler.callInstructionPrologue, "disconnect disables profiler while preserving debugger prologue");
  ares::Platform quietPlatform; auto* previousPlatform = ares::platform; ares::platform = &quietPlatform;
  prof.clearStats(); cpu.icache.power(false); pc(base + 0x300); cpu.jitClockTarget = cpu.clock + 100000;
  cpu.instruction(); cpu.instruction();
  check(cpu.recompiler.sections[cpu.recompiler.sectionIndex(0x1300)] != nullptr, "debugger-only JIT block compiled with inline cache refill");
  check(profView.execute("n64.profiler.control", {"1"}, "fixture", false).error.empty(), "enable profiler while instruction tracer already owns shared hook");
  check(cpu.recompiler.callInstructionPrologue && !cpu.recompiler.sections[cpu.recompiler.sectionIndex(0x1300)], "profiler toggle invalidates blocks even when shared prologue stays enabled");
  cpu.icache.power(false); pc(base + 0x300); cpu.jitClockTarget = cpu.clock + 100000;
  for(unsigned n = 0; n < 10 && prof.stats.empty(); ++n) cpu.instruction();
  check(prof.stats.count(0x80001340) && prof.stats.at(0x80001340).exclBytesIn == 32, "shared-tracer JIT counts actual cache-refill bytes after enabling profiling");
  profView.releaseOwner("fixture");
  cpu.debugger.tracer.instruction->setEnabled(false); check(!cpu.recompiler.callInstructionPrologue, "both disabled restores JIT without prologues");
  ares::platform = previousPlatform;
  prof.clearStats();
  CPU::Profiler::Frame boundedFrame; boundedFrame.funcAddr = 0x80001000;
  for(unsigned n = 0; n < CPU::Profiler::maxFunctions; ++n) prof.stats.emplace(n, CPU::Profiler::FuncStat{});
  prof.commitFrame(boundedFrame, nullptr, prof.now());
  check(prof.stats.size() == CPU::Profiler::maxFunctions && prof.droppedFunctionUpdates == 1, "profiler function-map cap reports dropped updates");
  prof.power();
  // Tap scheduling uses the coordinator's exact VI target and one cleanup path.
  TapService tap; InputService tapInput; tapInput.available = true;
  ExecutionService tapExecution; tapExecution.supported = true; tapExecution.frameKind = "vi";
  tapExecution.frame = 10; bool tapPaused = true;
  tapExecution.onStart = [&] { tapInput.start(1, tapExecution.frame); };
  tapExecution.onFinish = [&](const std::string& reason) { tap.finish(reason, tapInput, 1, tapExecution.frame); tapInput.finish(1, tapExecution.frame); };
  auto tapStart = tap.execute({"1", "0", "2", "1000", "0"}, "tap-owner", 1, tapPaused, tapInput, tapExecution);
  check(tapStart.error.empty() && tapInput.latched[0].buttons == 1 && tapExecution.active->target == 12, "tap button latched before first slice with exact target");
  tapExecution.frame = 11; tapExecution.service(tapPaused);
  check(tap.active() && tapInput.latched[0].buttons == 1, "tap remains held before target VI");
  tapExecution.frame = 12; tapExecution.service(tapPaused);
  check(!tap.active() && !tapInput.hasOverrides() && tapExecution.finished.back().second.find("\"held_frames\":2") != std::string::npos, "tap finishes exact interval and freezes held count");
  for(const char* reason : {"cancelled", "timeout", "breakpoint", "watchpoint", "paused", "disconnected", "host_input", "reset", "shutdown"}) {
    check(tap.execute({"1", "1", "2", "1000", "1"}, "tap-owner", 1, tapPaused, tapInput, tapExecution).error.empty(), "tap cleanup case starts");
    auto final = tapExecution.complete(reason, tapPaused);
    bool retain = TapService::retains(reason);
    check(!tap.active() && tapInput.pending[0].overridden() == retain && tapInput.latched[0].buttons == 0 && tapInput.pending[0].buttons == 0, "every tap exit releases button and revocation does not reacquire override");
    check(final.find(retain ? "\"ownership\":\"injected\"" : "\"ownership\":\"host\"") != std::string::npos, "tap immutable result matches cleanup ownership");
    tapInput.releaseAll();
  }
  check(tap.execute({"1", "14", "2", "1000", "0"}, "tap-owner", 1, tapPaused, tapInput, tapExecution).error == "invalid_request" && !tapInput.hasOverrides(), "invalid tap makes no input mutation");
  // Native image snapshots: actual VI RDRAM formats and render-thread epoch tags.
  FrameService images;
  vi.io.dramAddress = 0x4000; vi.io.width = 2; vi.io.colorDepth = 2; vi.io.vstart = 0; vi.io.vend = 4; vi.io.yscale = 0x400;
  const uint16_t colors[] = {0xf801, 0x07c1, 0x003f, 0xffff};
  for(unsigned n = 0; n < 4; ++n) rdram.ram.write<Half>(0x4000 + n * 2, colors[n], RBusDevice::ARES_DEBUGGER);
  auto framebuffer = images.execute("video.begin", {"framebuffer"}, "image-owner", 1, 10, true);
  check(framebuffer.error.empty() && images.pins.back().image->width == 2 && images.pins.back().image->height == 2 && images.pins.back().image->nonblack == 4, "VI framebuffer dimensions and nonblack count");
  auto raw = images.execute("video.read", {"1", "0", "16"}, "image-owner", 1, 10, true);
  check(raw.error.empty() && raw.json.find("ff0000ff00ff00ff0000ffffffffffff") != std::string::npos, "RGBA5551 converts big-endian RDRAM pixels to opaque RGBA8");
  check(images.execute("video.read", {"1", "0", "16"}, "foreign-owner", 1, 10, true).error == "busy", "native image pins are connection owned");
  rdram.ram.write<Half>(0x4000, 0, RBusDevice::ARES_DEBUGGER);
  check(images.execute("video.read", {"1", "0", "16"}, "image-owner", 1, 10, true).json == raw.json, "native framebuffer snapshot is immutable after RAM writes");
  check(images.execute("video.read", {"1", "15", "2"}, "image-owner", 1, 10, true).error == "out_of_bounds", "native pixel range bounds");
  check(images.execute("video.release", {"1"}, "image-owner", 1, 10, true).error.empty(), "native framebuffer release");
  vi.io.colorDepth = 3;
  rdram.ram.write<Word>(0x4000, 0x12345600, RBusDevice::ARES_DEBUGGER);
  check(images.execute("video.begin", {"framebuffer"}, "image-owner", 1, 10, true).error.empty(), "RGBA8888 framebuffer capture");
  check(images.execute("video.read", {"2", "0", "4"}, "image-owner", 1, 10, true).json.find("123456ff") != std::string::npos, "RGBA8888 preserves RGB and ignores source alpha like reference PNG");
  images.releaseOwner("image-owner"); check(images.pins.empty(), "owner revocation drops native image pins");
  vi.io.colorDepth = 0; check(images.execute("video.begin", {"framebuffer"}, "image-owner", 1, 10, true).error == "vi_blank", "blank VI does not fabricate a framebuffer");
  vi.io.colorDepth = 2; vi.io.width = 1; vi.io.vend = 2;
  check(images.execute("video.begin", {"framebuffer"}, "image-owner", 1, 10, true).error.empty(), "one-pixel RDRAM framebuffer remains valid (only presented 1x1 is invalid scanout)");
  images.releaseOwner("image-owner");
  const void* screenToken = &images;
  const uint8_t rgba[16] = {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255};
  images.submitted(screenToken, 1, 10); images.observe(screenToken, rgba, 2, 2, 1);
  check(images.latest && images.latest->epoch == 1 && images.latest->frame == 10, "native presented image uses emulation-thread submission tag");
  images.reset(2); images.observe(screenToken, rgba, 2, 2, 2);
  check(!images.latest && images.execute("video.begin", {"presented"}, "image-owner", 2, 0, true).error == "no_frame", "delayed pre-reset refresh cannot become a fresh image");
  images.submitted(screenToken, 2, 0); images.observe(screenToken, rgba, 2, 2, 3);
  check(images.execute("video.begin", {"presented"}, "image-owner", 2, 0, true).error.empty(), "new-epoch presented snapshot accepted");
  images.pins[0].expires = FrameService::Clock::now() - std::chrono::seconds(1);
  check(images.execute("video.read", {std::to_string(images.pins[0].id), "0", "4"}, "image-owner", 2, 0, true).error == "stale_snapshot", "expired native image pin revoked");
  pi.detach(cartridge.romDevice); cartridge.rom.reset();
  rdram.ram.reset();
  std::puts("CPU_DEBUG_FIXTURE_PASS");
}
