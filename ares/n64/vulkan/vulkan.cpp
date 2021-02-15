#include <n64/n64.hpp>

namespace ares::Nintendo64 {

Vulkan vulkan;

struct Vulkan::Implementation {
  Implementation(u8* data, u32 size);
  ~Implementation();

  ::Vulkan::Context context;
  ::Vulkan::Device device;
  ::RDP::CommandProcessor* processor = nullptr;

  //commands are u64 words, but the backend uses u32 swapped words.
  //size and offset are in u64 words.
  u32 buffer[0x10000] = {};
  u32 queueSize = 0;
  u32 queueOffset = 0;
};

auto Vulkan::load(Node::Object) -> bool {
  delete implementation;
  implementation = new Vulkan::Implementation(rdram.ram.data, rdram.ram.size);
  if(!implementation->processor) {
    delete implementation;
    implementation = nullptr;
  }
  return true;
}

auto Vulkan::unload() -> void {
  delete implementation;
  implementation = nullptr;
}

auto Vulkan::render() -> bool {
  if(!implementation) return false;

  static constexpr u32 commandLength[64] = {
    1, 1, 1, 1, 1, 1, 1, 1, 4, 6,12,14,12,14,20,22,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  };

  auto& command = rdp.command;
  auto& memory = !command.source ? rdram.ram : rsp.dmem;

  u32 current = command.current & ~7;
  u32 end = command.end & ~7;
  u32 length = (end - current) / 8;
  if(current >= end) return true;

  u32* buffer = implementation->buffer;
  u32& queueSize = implementation->queueSize;
  u32& queueOffset = implementation->queueOffset;
  if(queueSize + length >= 0x8000) return true;

  do {
    buffer[queueSize * 2 + 0] = memory.readWord(current); current += 4;
    buffer[queueSize * 2 + 1] = memory.readWord(current); current += 4;
    queueSize++;
  } while(--length);

  while(queueOffset < queueSize) {
    u32 op = buffer[queueOffset * 2];
    u32 code = op >> 24 & 63;
    u32 length = commandLength[code];

    if(queueOffset + length > queueSize) {
      //partial command, keep data around for next processing call
      command.start = command.current = command.end;
      return true;
    }

    if(code >= 8) {
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
  command.start = command.current = command.end;
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

auto Vulkan::scanout(std::vector<::RDP::RGBA>& colors, u32& width, u32& height) -> bool {
  if(!implementation) return false;
  implementation->processor->scanout_sync(colors, width, height);
  return true;
}

Vulkan::Implementation::Implementation(u8* data, u32 size) {
  if(!::Vulkan::Context::init_loader(nullptr)) return;
  if(!context.init_instance_and_device(nullptr, 0, nullptr, 0, ::Vulkan::CONTEXT_CREATION_DISABLE_BINDLESS_BIT)) return;
  device.set_context(context);
  device.init_frame_contexts(3);
  processor = new ::RDP::CommandProcessor(device, data, 0, size, size / 2, 0);
  if(!processor->device_is_supported()) {
    delete processor;
    processor = nullptr;
  }
}

Vulkan::Implementation::~Implementation() {
  delete processor;
}

}
