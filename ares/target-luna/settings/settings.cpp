#include "../luna.hpp"
Settings settings;

auto Settings::load() -> void {
  if(auto document = file::read({Path::userSettings(), "luna/settings.bml"})) {
    properties(0, BML::unserialize(document));
  }
}

auto Settings::save() -> void {
  Markup::Node document;
  properties(1, document);
  directory::create({Path::userSettings(), "luna/"});
  file::write({Path::userSettings(), "luna/settings.bml"}, BML::serialize(document));
}

auto Settings::properties(bool mode, Markup::Node document) -> void {
  #define s(name) \
    if(mode == 0 && document[string{#name}.trimRight(".identifier", 1L).transform(".", "/")]) { \
      document[string{#name}.trimRight(".identifier", 1L).transform(".", "/")].value(name); \
    } \
    if(mode == 1) { \
      document(string{#name}.trimRight(".identifier", 1L).transform(".", "/")).setValue(name); \
    }
  s(video.driver)
  s(video.monitor)
  s(video.format)
  s(video.exclusive)
  s(video.blocking)
  s(video.flush)
  s(video.luminance)
  s(video.saturation)
  s(video.gamma)
  s(video.output)
  s(video.aspectCorrection)
  s(video.scale)
  s(video.shader)
  s(audio.driver)
  s(audio.device)
  s(audio.frequency)
  s(audio.latency)
  s(audio.exclusive)
  s(audio.blocking)
  s(audio.dynamic)
  s(audio.skew)
  s(audio.volume)
  s(audio.balance)
  s(audio.mute)
  s(input.driver)
  s(input.unfocused)
  s(interface.showStatusBar)
  s(interface.showSystemPanels)
  s(interface.advancedMode)
  s(hotkeys.toggleStatus.identifier)
  s(hotkeys.togglePanels.identifier)
  s(hotkeys.toggleFullscreen.identifier)
  s(hotkeys.toggleMouseCapture.identifier)
  s(hotkeys.fastForward.identifier)
  s(hotkeys.saveState.identifier)
  s(hotkeys.loadState.identifier)
  s(hotkeys.incrementStateSlot.identifier)
  s(hotkeys.decrementStateSlot.identifier)
  s(hotkeys.pauseEmulation.identifier)
  s(hotkeys.exportMemory.identifier)
  s(hotkeys.quitEmulator.identifier)
  #undef s
}
