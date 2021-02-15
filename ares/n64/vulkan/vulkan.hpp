#include "rdp_device.hpp"

namespace ares::Nintendo64 {

struct Vulkan {
  auto load(Node::Object) -> bool;
  auto unload() -> void;

  auto render() -> bool;
  auto frame() -> void;
  auto writeWord(u32 address, u32 data) -> void;
  auto scanout(std::vector<::RDP::RGBA>& colors, u32& width, u32& height) -> bool;

  struct Implementation;
  Implementation* implementation = nullptr;
};

extern Vulkan vulkan;

}
