#pragma once

namespace ares {

enum class Event : u32 {
  None,
  Step,
  Frame,
  Power,
  Synchronize,
};

struct Platform {
  virtual auto attach(Node::Object) -> void {}
  virtual auto detach(Node::Object) -> void {}
  virtual auto pak(Node::Object) -> shared_pointer<vfs::directory> { return {}; }
  virtual auto event(Event) -> void {}
  virtual auto log(Node::Debugger::Tracer::Tracer, string_view message) -> void {}
  virtual auto status(string_view message) -> void {}
  virtual auto video(Node::Video::Screen, const u32* data, u32 pitch, u32 width, u32 height) -> void {}
  virtual auto refreshRateHint(Node::Video::Screen, double refreshRate) -> void {}
  virtual auto resetPalette(ares::Node::Video::Screen node) -> void {}
  virtual auto resetSprites(ares::Node::Video::Screen node) -> void {}
  virtual auto setViewport(ares::Node::Video::Screen node, u32 x, u32 y, u32 width, u32 height) -> void {}
  virtual auto setOverscan(ares::Node::Video::Screen node, bool overscan) -> void {}
  virtual auto setSize(ares::Node::Video::Screen node, u32 width, u32 height) -> void {}
  virtual auto setScale(ares::Node::Video::Screen node, f64 scaleX, f64 scaleY) -> void {}
  virtual auto setAspect(ares::Node::Video::Screen node, f64 aspectX, f64 aspectY) -> void {}
  virtual auto setSaturation(ares::Node::Video::Screen node, f64 saturation) -> void {}
  virtual auto setGamma(ares::Node::Video::Screen node, f64 gamma) -> void {}
  virtual auto setLuminance(ares::Node::Video::Screen node, f64 luminance) -> void {}
  virtual auto setFillColor(ares::Node::Video::Screen node, u32 fillColor) -> void {}
  virtual auto setColorBleed(ares::Node::Video::Screen node, bool colorBleed) -> void {}
  virtual auto setColorBleedWidth(ares::Node::Video::Screen node, u32 width) -> void {}
  virtual auto setInterframeBlending(ares::Node::Video::Screen node, bool interframeBlending) -> void {}
  virtual auto setRotation(ares::Node::Video::Screen node, u32 rotation) -> void {}
  virtual auto setProgressive(ares::Node::Video::Screen node, bool progressiveDouble) -> void {}
  virtual auto setInterlace(ares::Node::Video::Screen node, bool interlaceField) -> void {}
  virtual auto attachSprite(ares::Node::Video::Screen node, ares::Node::Video::Sprite sprite) -> void {}
  virtual auto detachSprite(ares::Node::Video::Screen node, ares::Node::Video::Sprite sprite) -> void {}
  virtual auto colors(ares::Node::Video::Screen node, u32 colors, function<n64 (n32)> color) -> void {}
  
  virtual auto audio(Node::Audio::Stream) -> void {}
  virtual auto input(Node::Input::Input) -> void {}
  virtual auto cheat(u32 addr) -> maybe<u32> { return nothing; }
};

extern Platform* platform;

}

namespace ares::Core {
  // <ares/node/node.hpp> forward declarations
  auto PlatformAttach(Node::Object node) -> void { if(platform && node->name()) platform->attach(node); }
  auto PlatformDetach(Node::Object node) -> void { if(platform && node->name()) platform->detach(node); }
  auto PlatformLog(Node::Debugger::Tracer::Tracer node, string_view text) -> void { if(platform) platform->log(node, text); }
}
