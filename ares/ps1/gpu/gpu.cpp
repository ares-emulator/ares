#include <ps1/ps1.hpp>
#include <array>

namespace ares::PlayStation {

GPU gpu;
GPU::Color GPU::Color::table[65536];

#include "io.cpp"
#include "gp0.cpp"
#include "gp1.cpp"
#include "renderer.cpp"
#include "scanout.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto GPU::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("GPU");
  pixelAccuracySetting = node->append<Node::Setting::Boolean>("Pixel Accuracy", pixelAccuracy);

  screen = node->append<Node::Video::Screen>("Screen", Scanout::OutputPitch, Scanout::OutputHeight);
  screen->colors((1 << 24) + (1 << 15), [&](n32 color) -> n64 {
    if(color < (1 << 24)) {
      u64 a = 65535;
      u64 r = image::normalize(color >>  0 & 255, 8, 16);
      u64 g = image::normalize(color >>  8 & 255, 8, 16);
      u64 b = image::normalize(color >> 16 & 255, 8, 16);
      return a << 48 | r << 32 | g << 16 | b << 0;
    } else {
      u64 a = 65535;
      u64 r = image::normalize(color >>  0 & 31, 5, 16);
      u64 g = image::normalize(color >>  5 & 31, 5, 16);
      u64 b = image::normalize(color >> 10 & 31, 5, 16);
      return a << 48 | r << 32 | g << 16 | b << 0;
    }
  });
  screen->setSize(Scanout::OutputPitch, Scanout::OutputHeight);

  vram.allocate(1_MiB);
  for(u32 y : range(512)) {
    vram2D[y] = (u16*)&vram.data[y * 2048];
  }

  debugger.load(node);

  generateTables();
}

auto GPU::unload() -> void {
  renderer.kill();
  debugger = {};
  vram.reset();
  node->remove(screen);
  screen.reset();
  pixelAccuracySetting.reset();
  node.reset();
}

auto GPU::main() -> void {
  u32 htotal = nextHtotal();
  u32 timerHblankClock = min(hblankStart(), htotal);
  io.hcounter = 0;

  io.dotcounter = 0;
  io.hblankClock = timerHblankClock;
  io.hblankFired = timerHblankClock >= htotal;
  scanout.lineStart();

  step(htotal);
  io.hcounter = htotal;
  scanout.lineEnd();
  timer.hsync(0);
  io.hcounter = 0;

  bool frameEnd = ++io.vcounter >= vtotal();
  if(frameEnd) {
    io.vcounter = 0;
    io.field = !io.field;
    frame();
  }

  if(io.vcounter == vstart()) {
    timer.vsync(0);
    interrupt.lower(Interrupt::Vblank);
  }

  if(io.vcounter == vend() && !interrupt.vblank.line) {
    io.displayField = io.interlace ? !io.field : false;
    timer.vsync(1);
    interrupt.raise(Interrupt::Vblank);
    scanout.queue();
  }

  if(frameEnd) {
    if(!refreshed) scanout.queue();
    refreshed = false;
  }
}

auto GPU::nextHtotal() -> u32 {
  u32 scale = halfLine() ? 1 : 2;
  u64 numerator = htotalNumerator() * scale + io.htotalRemainder;
  u32 denominator = htotalDenominator() * 2;
  u32 htotal = numerator / denominator;
  io.htotalRemainder = numerator % denominator;
  return htotal;
}

auto GPU::frame() -> void {
  display.dotclock = dotclockDivider();
  display.width = displayWidth();

  if(io.verticalResolution && io.interlace) {
    display.height = io.videoMode ? 512 : 480;
    display.interlace = 1;
  } else {
    display.height = io.videoMode ? 256 : 240;
    display.interlace = 0;
  }
}

auto GPU::step(u32 clocks) -> void {
  static constexpr u32 SyncQuantum = 128;
  const double clocksPerTime = 1.0 / (double)scalar();
  bool timerDotclock = timer.timers[0].clock == 1;
  bool needsPixelStepping = timerDotclock || scanout.lineVisible || io.dividerClocks;

  auto advanceCounter = [](i32& counter, u32 stepClocks, u32 div) -> void {
    if(div == 0) return;
    if(counter < (s32)div) {
      u32 first = div - counter;
      if(stepClocks < first) {
        counter += stepClocks;
      } else {
        counter = (stepClocks - first) % div;
      }
    } else {
      u32 first = (16 - counter) + div;
      if(stepClocks < first) {
        counter = (counter + stepClocks) & 15;
      } else {
        counter = (stepClocks - first) % div;
      }
    }
  };

  if(needsPixelStepping) {
    u32 syncClocks = 0;
    bool skipDMAWait = false;
    while(clocks) {
      timerDotclock = timer.timers[0].clock == 1;
      bool stableSpan = scanout.lineVisible && !io.dividerClocks
        && (scanout.displayCountLatched || (!scanout.accurate && !timerDotclock));
      u32 stepClocks = clocks;
      if(scanout.lineVisible) {
        if(!syncClocks) {
          skipDMAWait = dma.canSkipWait();
          u64 dmaDeadline = skipDMAWait ? cpu.clock() : dma.clock();
          u64 deadline = min(cpu.clock(), dmaDeadline, spu.clock(), mdec.clock());
          syncClocks = 1;
          if(deadline >= clock()) {
            u64 delta = deadline - clock();
            if(delta >= (u64)clocks * scalar()) {
              syncClocks = clocks;
            } else {
              u32 ticks = (double)delta * clocksPerTime;
              while((u64)ticks * scalar() > delta) --ticks;
              while((u64)(ticks + 1) * scalar() <= delta) ++ticks;
              syncClocks = ticks + 1;
            }
          }
        }
        stepClocks = min(stepClocks, syncClocks);
        if(!scanout.displayCountLatched) {
          u32 start = io.displayRangeX1;
          stepClocks = min(stepClocks, start > io.hcounter ? start - io.hcounter : 1u);
        }
      }
      if(io.dividerClocks || fill.active || (io.pcounter == 0 && (latch.ready || !fifo.empty()))) stepClocks = 1;
      if(io.pcounter > 0) stepClocks = min<u32>(stepClocks, (u32)io.pcounter);
      if(!io.dividerClocks && !stableSpan) {
        u32 div = dotclockDivider();
        if(div == 0) div = 1;
        u32 toMatch = io.timerDotcounter < (s32)div ? div - io.timerDotcounter : (16 - io.timerDotcounter) + div;
        stepClocks = min(stepClocks, toMatch);
        u32 displayMatch = io.dotcounter < (s32)div ? div - io.dotcounter : 16 - io.dotcounter + div;
        stepClocks = min(stepClocks, displayMatch);
      }
      if(!io.hblankFired && io.hcounter < io.hblankClock) stepClocks = min(stepClocks, io.hblankClock - io.hcounter);

      clocks -= stepClocks;
      if(stableSpan) {
        u32 div = dotclockDivider();
        if(!div) div = 1;
        if(scanout.accurate) {
          u32 first = io.dotcounter < (s32)div ? div - io.dotcounter : 16 - io.dotcounter + div;
          u32 end = io.hcounter + stepClocks;
          for(u32 edge = io.hcounter + first; edge <= end; edge += div) scanout.pixel(edge);
        }

        if(timerDotclock) {
          u32 timerFirst = io.timerDotcounter < (s32)div ? div - io.timerDotcounter : 16 - io.timerDotcounter + div;
          if(stepClocks >= timerFirst) timer.timers[0].step(1 + (stepClocks - timerFirst) / div);
        }
        advanceCounter(io.dotcounter, stepClocks, div);
        advanceCounter(io.timerDotcounter, stepClocks, div);
        if(!scanout.displayCountLatched) scanout.clock(io.hcounter + stepClocks);
      } else {
        if(io.dividerClocks) advanceDivider();
        io.dotcounter = (io.dotcounter + stepClocks) & 15;
        io.timerDotcounter = (io.timerDotcounter + stepClocks) & 15;

        if(scanout.lineVisible) scanout.clock(io.hcounter + stepClocks);

        u32 displayDiv = dotclockDivider();
        if(displayDiv == 0) displayDiv = 1;

        if(io.dotcounter == (s32)displayDiv) {
          u32 pixelClock = io.hcounter + stepClocks;
          io.dotcounter = 0;
          if(scanout.lineVisible && scanout.accurate) scanout.pixel(pixelClock);
        }

        u32 timerDiv = dotclockDivider();
        if(timerDiv == 0) timerDiv = 1;
        if(io.timerDotcounter == (s32)timerDiv) {
          io.timerDotcounter = 0;
          if(timerDotclock) timer.timers[0].step();
        }

      }

      io.hcounter += stepClocks;

      if(!io.hblankFired && io.hcounter >= io.hblankClock) {
        io.hblankFired = true;
        timer.hsync(1);
      }

      if(unlikely(fill.active)) stepFill(stepClocks);
      Thread::step(stepClocks);

      if(scanout.lineVisible) dma.skipBusWait(clock(), skipDMAWait);
      io.pcounter -= stepClocks;
      if(io.pcounter < 0) io.pcounter = 0;
      if(io.pcounter == 0) finishPendingRender();
      if(io.pcounter == 0 && (latch.ready || !fifo.empty())) {
        stepFifo();
        if(scanout.lineVisible) syncClocks = stepClocks;
      }
      if(scanout.lineVisible) {
        syncClocks -= stepClocks;
        if(!syncClocks) {
          Thread::synchronize();
        }
      } else {
        Thread::synchronize();
      }
    }
    return;
  }

  while(clocks) {
    if(io.dividerClocks) { step(clocks); return; }
    u32 stepClocks = min<u32>(clocks, SyncQuantum);
    if(io.pcounter > 0) stepClocks = min<u32>(stepClocks, (u32)io.pcounter);
    if(!io.hblankFired && io.hcounter + stepClocks > io.hblankClock) stepClocks = io.hblankClock - io.hcounter;
    clocks -= stepClocks;

    u32 displayDiv = dotclockDivider();
    advanceCounter(io.dotcounter, stepClocks, displayDiv);

    u32 timerDiv = dotclockDivider();
    advanceCounter(io.timerDotcounter, stepClocks, timerDiv);

    io.hcounter += stepClocks;

    if(!io.hblankFired && io.hcounter >= io.hblankClock) {
      io.hblankFired = true;
      timer.hsync(1);
    }

    if(unlikely(fill.active)) stepFill(stepClocks);
    Thread::step(stepClocks);
    io.pcounter -= stepClocks;
    if(io.pcounter < 0) io.pcounter = 0;
    if(io.pcounter == 0) finishPendingRender();
    if(io.pcounter == 0 && (latch.ready || !fifo.empty())) stepFifo();
    Thread::synchronize();
  }
}

auto GPU::power(bool reset) -> void {
  Thread::create(system.gpuFrequency(), std::bind_front(&GPU::main, this));
  screen->power();
  refreshed = false;

  random.array({vram.data, vram.size});
  display.dotclock = 10;
  display.width = 256;
  display.height = 240;
  display.interlace = 0;
  display.previous.x = 0;
  display.previous.y = 0;
  display.previous.width = 0;
  display.previous.height = 0;
  io = {};
  pendingTextureTiming = false;
  fill = {};
  fifo = {};
  ingress = {};
  latch = {};
  queue.gp0 = {};
  queue.gp1 = {};

  frame();
  renderer.power();
  invalidateCache();
  scanout.power();
}

auto GPU::advanceDivider() -> void {
  io.dividerPipeline[2] = io.dividerPipeline[1];
  io.dividerPipeline[1] = io.dividerPipeline[0];
  io.dividerPipeline[0] = rawDotclockDivider();
  --io.dividerClocks;
}

}
