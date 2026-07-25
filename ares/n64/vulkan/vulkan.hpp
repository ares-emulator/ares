#include "rdp_device.hpp"

namespace ares::Nintendo64 {

struct RDPFrameCapture;
//This header precedes memory.hpp; the replay RDRAM is only dereferenced by
//translation units that include the full n64.hpp.
namespace Memory { struct Writable; }

struct Vulkan {
  auto load(Node::Object) -> bool;
  auto unload() -> void;

  auto render() -> bool;
  auto frame() -> void;
  auto writeWord(u32 address, u32 data) -> void;
  auto scanoutAsync(bool field) -> bool;
  auto mapScanoutRead(const u8*& rgba, u32& width, u32& height) -> void;
  auto unmapScanoutRead() -> void;
  auto endScanout() -> void;
  auto crashed() -> const char*;
  auto synchronize() -> void;
  auto readTMEM(u32 address) -> u8;
  auto copyTMEM(u8* target, u32 size) -> bool;
  auto replay(const RDPFrameCapture& capture, u32 command) -> bool;
  auto replayRdram() const -> Memory::Writable*;
  auto replayHiddenRdram() const -> const u8*;
  auto replayTMEM() const -> const u8*;
  auto replayCrashed() const -> const char*;

  struct Implementation;
  Implementation* implementation = nullptr;

  bool enable = true;
  bool disableVideoInterfaceProcessing = false;
  bool weaveDeinterlacing = false;
  u32  internalUpscale = 1;  //1, 2, 4, 8
  bool supersampleScanout = false;
  u32  outputUpscale = supersampleScanout ? 1 : internalUpscale;
};

extern Vulkan vulkan;

}
