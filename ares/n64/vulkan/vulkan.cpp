#include <n64/n64.hpp>

//included here rather than in vulkan.hpp so that parallel-rdp headers are only
//parsed in this translation unit
#include "rdp_device.hpp"

namespace ares::Nintendo64 {

Vulkan vulkan;

struct LoggingInterface : Util::LoggingInterface {
  auto log(const char* tag, const char* fmt, va_list va) -> bool {
    char buffer[8192];
    vsnprintf(buffer, sizeof(buffer), fmt, va);
  //print(terminal::color::yellow(tag), buffer);
    return true;
  }
} loggingInterface;

struct Vulkan::Implementation {
  Implementation(u8* data, u32 size);
  ~Implementation();

  ::Vulkan::Context context;
  ::Vulkan::Device device;
  ::RDP::CommandProcessor* processor = nullptr;
  u8* tmem = nullptr;
  ::Vulkan::Context replayContext;
  ::Vulkan::Device replayDevice;
  bool replayDeviceInitialized = false;
  ::RDP::CommandProcessor* replayProcessor = nullptr;
  Memory::Writable replayRam;
  const u8* replayHidden = nullptr;
  const u8* replayTmem = nullptr;
  u64 replayIdentifier = 0;
  u32 replayPacket = 0;
  i32 replayCommand = -1;
  atomic<const char*> crash_error = nullptr;
  atomic<const char*> replayCrashError = nullptr;

  //Messages are static literals from rdp_renderer.cpp, so storing the pointer
  //is safe across threads.
  struct Validation : public ::RDP::ValidationInterface {
    atomic<const char*>& target;
    Validation(atomic<const char*>& target) : target(target) {}
    void report_rdp_crash(::RDP::ValidationError err, const char *msg) override {
      target = msg;
    }
  } validator{crash_error}, replayValidator{replayCrashError};

  //commands are u64 words, but the backend uses u32 swapped words.
  //size and offset are in u64 words.
  u32 buffer[0x10000] = {};
  u32 queueSize = 0;
  u32 queueOffset = 0;

  ::RDP::VIScanoutBuffer scanout;
  std::mutex lock;
  std::condition_variable condition;
  u32 scanoutCount = 0;
  u32 endCount = 0;
};

auto Vulkan::load(Node::Object) -> bool {
  if (vulkan.enable) {
    Util::set_thread_logging_interface(&loggingInterface);
    delete implementation;
    implementation = new Vulkan::Implementation(rdram.ram.data, rdram.ram.size);
    if(!implementation->processor) {
      delete implementation;
      implementation = nullptr;
    }

    if (!implementation) {
      platform->status("Vulkan init failed: No RDP rendering support");
      vulkan.enable = false;
      rdram.hidden.data = nullptr;
    } else {
      platform->status("Vulkan Enabled: using paraLLEl-RDP");
      rdram.hidden.data = (u8*)implementation->processor->begin_read_hidden_rdram();
    }
  } else {
    platform->status("Vulkan Disabled: No RDP rendering support");
    rdram.hidden.data = nullptr;
  }

  return true;
}

auto Vulkan::unload() -> void {
  rdram.hidden.data = nullptr;
  if (implementation) delete implementation;
  implementation = nullptr;
}

auto Vulkan::render() -> bool {
  if(!implementation) return false;

  auto& command = rdp.command;

  u32 current = command.current & ~7;
  u32 end = command.end & ~7;
  u32 length = (end - current) / 8;
  if(current >= end) return true;

  u32* buffer = implementation->buffer;
  u32& queueSize = implementation->queueSize;
  u32& queueOffset = implementation->queueOffset;
  if(queueSize + length >= 0x8000) return true;

  if(!command.source) {
    do {
      buffer[queueSize * 2 + 0] = rdram.ram.read<Word>(current, RBusDevice::DP_DMA); current += 4;
      buffer[queueSize * 2 + 1] = rdram.ram.read<Word>(current, RBusDevice::DP_DMA); current += 4;
      queueSize++;
    } while(--length);
  } else {
    do {
      buffer[queueSize * 2 + 0] = rsp.dmem.read<Word>(current); current += 4;
      buffer[queueSize * 2 + 1] = rsp.dmem.read<Word>(current); current += 4;
      if(system.homebrewMode) {
        rsp.debugger.dmemReadWord(current - 8, 8, "RDP XBUS");
      }
      queueSize++;
    } while(--length);
  }

  while(queueOffset < queueSize) {
    u32 op = buffer[queueOffset * 2];
    u32 code = op >> 24 & 63;
    u32 length = rdpCommandLength(code);

    if(queueOffset + length > queueSize) {
      //partial command, keep data around for next processing call
      command.start = command.current = command.end;
      return true;
    }

    if(unlikely(rdp.debugger.captureActive())) {
      rdp.debugger.commandBatch();
      rdp.debugger.captureCommand(buffer + queueOffset * 2, length * 2);
    }
    if(code >= 8) {
      rdp.debugger.observeCommand(code, buffer + queueOffset * 2, length * 2);
      implementation->processor->enqueue_command(length * 2, buffer + queueOffset * 2);
    }

    if(::RDP::Op(code) == ::RDP::Op::SyncFull) {
      implementation->processor->wait_for_timeline(implementation->processor->signal_timeline());
      rdp.syncFull();
    }

    queueOffset += length;
  }

  queueOffset = 0;
  queueSize = 0;
  command.current = command.end;
  return true;
}

auto Vulkan::frame() -> void {
  if(!implementation) return;
  implementation->processor->begin_frame_context();
}

auto Vulkan::writeWord(u32 address, u32 data) -> void {
  if(!implementation) return;
  implementation->processor->set_vi_register(::RDP::VIRegister(address), data);
}

auto Vulkan::scanoutAsync(bool field) -> bool {
  if(!implementation) return false;

  { //wait until we're done reading in thread before we clobber the readback buffer
    std::unique_lock<std::mutex> lock{implementation->lock};
    implementation->condition.wait(lock, [this]() {
      return implementation->scanoutCount == implementation->endCount;
    });
  }

  implementation->processor->set_vi_register(::RDP::VIRegister::VCurrentLine, field);

  //0 steps if scanning out at upscaled resolution.
  //each downscale step reduces output resolution to [width, height] * max(1, upscale >> downscale_steps)
  ::RDP::ScanoutOptions options;
  options.downscale_steps = supersampleScanout ? 16 : 0;
  options.persist_frame_on_invalid_input = true;  //this is a compatibility hack, but I'm not sure what for ...
  if(disableVideoInterfaceProcessing) {
    options.vi = {false, false, true, false, false, false};
  }
  if(!supersampleScanout){
    options.blend_previous_frame = weaveDeinterlacing;
    options.upscale_deinterlacing = !weaveDeinterlacing;
  }
  else {
    options.blend_previous_frame = false;
    options.upscale_deinterlacing = true;
  }


  if(implementation->scanout.fence) {
    implementation->scanout.fence->wait();
  }
  implementation->processor->scanout_async_buffer(implementation->scanout, options);
  implementation->scanoutCount++;
  return true;
}

auto Vulkan::mapScanoutRead(const u8*& rgba, u32& width, u32& height) -> void {
  if(!implementation || !implementation->scanout.fence || !implementation->scanout.width || !implementation->scanout.height) {
    rgba = nullptr;
    width = 0;
    height = 0;
  } else {
    implementation->scanout.fence->wait();
    rgba = (const u8*)implementation->device.map_host_buffer(*implementation->scanout.buffer, ::Vulkan::MEMORY_ACCESS_READ_BIT);
    width = implementation->scanout.width;
    height = implementation->scanout.height;
  }
}

auto Vulkan::unmapScanoutRead() -> void {
  if(implementation && implementation->scanout.buffer) {
    implementation->device.unmap_host_buffer(*implementation->scanout.buffer, ::Vulkan::MEMORY_ACCESS_READ_BIT);
  }
}

auto Vulkan::endScanout() -> void {
  if(implementation) {
    //notify main thread that we're done reading
    std::lock_guard<std::mutex> lock{implementation->lock};
    implementation->endCount++;
    implementation->condition.notify_one();
  }
}

auto Vulkan::crashed() -> const char* {
  if(implementation) return implementation->crash_error;
  return nullptr;
}

auto Vulkan::synchronize() -> void {
  if(implementation && implementation->processor) {
    implementation->processor->idle();
  }
}

//parallel-RDP holds TMEM as an array of native-endian 16-bit words whose index is
//swizzled the same way RDRAM is: logical 16-bit word N lives at index N^1. Reading
//that buffer as bytes on a little-endian host therefore needs a byte address XOR 3
//to recover TMEM's own big-endian byte order (compare the "index ^= 3" / "index ^= 1"
//in parallel-rdp/shaders/texture.h). address here is a logical TMEM byte address.
auto Vulkan::readTMEM(u32 address) -> u8 {
  if(!implementation || !implementation->tmem || address >= 4_KiB) return 0;
  return implementation->tmem[address ^ 3];
}

//Verbatim copy of the TMEM buffer in parallel-RDP's own layout, for seeding a replay
//processor through set_tmem(). Anything meant for display wants readTMEM() instead.
auto Vulkan::copyTMEM(u8* target, u32 size) -> bool {
  if(!implementation || !implementation->tmem) return false;
  memory::copy(target, implementation->tmem, min(size, 4_KiB));
  return true;
}

auto Vulkan::replay(const RDPFrameCapture& capture, u32 targetCommand) -> bool {
  if(!implementation || targetCommand >= capture.commandOffsets.size()) return false;
  auto& impl = *implementation;

  bool restart = !impl.replayProcessor
    || impl.replayIdentifier != capture.identifier
    || (i32)targetCommand < impl.replayCommand;
  if(restart) {
    if(!impl.replayDeviceInitialized) {
      if(!impl.replayContext.init_instance_and_device(nullptr, 0, nullptr, 0, 0)) return false;
      impl.replayDevice.set_context(impl.replayContext);
      impl.replayDevice.init_frame_contexts(3);
      impl.replayDeviceInitialized = true;
    }

    //The processor owns GPU-side renderer state but its complete RDP register
    //state can be overwritten by initialCommands. Keep it alive when scrubbing
    //backward; reconstructing it is expensive and emits backend initialization
    //messages every time.
    bool recreate = !impl.replayProcessor || impl.replayRam.size != capture.rdram.size();
    if(recreate) {
      delete impl.replayProcessor;
      impl.replayProcessor = nullptr;
      impl.replayRam.allocate(capture.rdram.size());
      auto flags = ::RDP::CommandProcessorFlags(
        ::RDP::COMMAND_PROCESSOR_FLAG_HOST_VISIBLE_HIDDEN_RDRAM_BIT |
        ::RDP::COMMAND_PROCESSOR_FLAG_HOST_VISIBLE_TMEM_BIT
      );
      impl.replayProcessor = new ::RDP::CommandProcessor(
        impl.replayDevice, impl.replayRam.data, 0, impl.replayRam.size,
        impl.replayRam.size / 2, flags
      );
      if(!impl.replayProcessor->device_is_supported()) {
        delete impl.replayProcessor;
        impl.replayProcessor = nullptr;
        return false;
      }
      impl.replayProcessor->set_validation_interface(&impl.replayValidator);
      impl.replayProcessor->begin_frame_context();
    } else {
      impl.replayProcessor->idle();
    }

    std::memcpy(impl.replayRam.data, capture.rdram.data(), impl.replayRam.size);
    impl.replayIdentifier = capture.identifier;
    impl.replayPacket = 0;
    impl.replayCommand = -1;
    impl.replayHidden = nullptr;
    impl.replayTmem = nullptr;

    impl.replayCrashError = nullptr;
    auto hidden = (u8*)impl.replayProcessor->begin_read_hidden_rdram();
    if(hidden && capture.hiddenRdram.size() == impl.replayRam.size / 2) {
      std::memcpy(hidden, capture.hiddenRdram.data(), capture.hiddenRdram.size());
    }
    impl.replayProcessor->end_write_hidden_rdram();
    if(capture.tmem.size() == 4_KiB) {
      impl.replayProcessor->set_tmem(capture.tmem.data(), capture.tmem.size());
    }
    for(auto& words : capture.initialCommands) {
      if(!words.empty()) {
        impl.replayProcessor->enqueue_command(words.size(), words.data());
      }
    }
    impl.replayProcessor->idle();
  }

  if((i32)targetCommand == impl.replayCommand) return true;
  for(; impl.replayPacket < capture.packets.size(); impl.replayPacket++) {
    auto& packet = capture.packets[impl.replayPacket];
    if(packet.type == RDPFrameCapture::Packet::Type::DramDiff) {
      bool beginsDiffGroup = impl.replayPacket == 0
        || capture.packets[impl.replayPacket - 1].type
          != RDPFrameCapture::Packet::Type::DramDiff;
      if(beginsDiffGroup) impl.replayProcessor->idle();
      if(packet.address + packet.bytes.size() <= impl.replayRam.size) {
        std::memcpy(
          impl.replayRam.data + packet.address,
          packet.bytes.data(), packet.bytes.size()
        );
      }
      continue;
    }
    if(packet.type == RDPFrameCapture::Packet::Type::ViRegister) {
      impl.replayProcessor->set_vi_register(::RDP::VIRegister(packet.address), packet.value);
      continue;
    }
    if(packet.type != RDPFrameCapture::Packet::Type::Commands) continue;

    impl.replayProcessor->enqueue_command(packet.words.size(), packet.words.data());
    impl.replayCommand++;
    if(impl.replayCommand == (i32)targetCommand) {
      impl.replayPacket++;
      break;
    }
  }
  impl.replayProcessor->idle();
  impl.replayHidden = (const u8*)impl.replayProcessor->begin_read_hidden_rdram();
  impl.replayTmem = (const u8*)impl.replayProcessor->get_tmem();
  return impl.replayCommand == (i32)targetCommand;
}

auto Vulkan::replayRdram() const -> Memory::Writable* {
  if(!implementation || !implementation->replayProcessor) return nullptr;
  return &implementation->replayRam;
}

auto Vulkan::replayHiddenRdram() const -> const u8* {
  if(!implementation) return nullptr;
  return implementation->replayHidden;
}

auto Vulkan::replayTMEM() const -> const u8* {
  if(!implementation) return nullptr;
  return implementation->replayTmem;
}

auto Vulkan::replayCrashed() const -> const char* {
  if(!implementation) return nullptr;
  return implementation->replayCrashError;
}

Vulkan::Implementation::Implementation(u8* data, u32 size) {
  if(!::Vulkan::Context::init_loader(nullptr)) return;
  if(!context.init_instance_and_device(nullptr, 0, nullptr, 0, 0)) return;
  device.set_context(context);
  device.init_frame_contexts(3);

  //TODO: Keep TMEM device-local unless a debugger view needs it. The debugger node
  //API cannot currently report whether its panel is open, so this 4 KiB allocation
  //is host-visible unconditionally. Replace this with an on-demand
  //staging readback (and a cached snapshot for the Memory node).
  ::RDP::CommandProcessorFlags flags =
    ::RDP::COMMAND_PROCESSOR_FLAG_HOST_VISIBLE_HIDDEN_RDRAM_BIT |
    ::RDP::COMMAND_PROCESSOR_FLAG_HOST_VISIBLE_TMEM_BIT;
  switch(vulkan.internalUpscale) {
  case 2: flags |= ::RDP::COMMAND_PROCESSOR_FLAG_UPSCALING_2X_BIT; break;
  case 4: flags |= ::RDP::COMMAND_PROCESSOR_FLAG_UPSCALING_4X_BIT; break;
  case 8: flags |= ::RDP::COMMAND_PROCESSOR_FLAG_UPSCALING_8X_BIT; break;
  }

  if(vulkan.internalUpscale > 1) {
    flags |= ::RDP::COMMAND_PROCESSOR_FLAG_SUPER_SAMPLED_DITHER_BIT;
    //rasky: this is explicitly disabled because we want to make sure we don't
    // read back the super sampled version, as it can cause artifacts. We want
    // parallelRDP to also produce a 1x render to use for readbacks.
    //flags |= ::RDP::COMMAND_PROCESSOR_FLAG_SUPER_SAMPLED_READ_BACK_BIT;
  }

  processor = new ::RDP::CommandProcessor(device, data, 0, size, size / 2, flags);
  if(!processor->device_is_supported()) {
    delete processor;
    processor = nullptr;
    return;
  }

  processor->set_validation_interface(&validator);
  tmem = (u8*)processor->get_tmem();
}

Vulkan::Implementation::~Implementation() {
  if(replayProcessor) delete replayProcessor;
  replayRam.reset();
  if(processor) delete processor;
}

}
