#include "runtime.hpp"

namespace ruby { Input input; }
FrontendRuntime runtime;

auto updatePalette() -> void {
  if(!emulator || !emulator->root) return;
  for(auto& screen : emulator->root->find<ares::Node::Video::Screen>()) {
    screen->setLuminance(settings.video.luminance);
    screen->setSaturation(settings.video.saturation);
    screen->setGamma(settings.video.gamma);
  }
}
