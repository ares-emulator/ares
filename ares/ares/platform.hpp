#pragma once

namespace ares {

enum class Event : uint {
  None,
  Step,
  Frame,
  Power,
  Synchronize,
};

struct Platform {
  virtual auto attach(Node::Object) -> void {}
  virtual auto detach(Node::Object) -> void {}
  virtual auto open(Node::Object, string name, vfs::file::mode mode, bool required = false) -> shared_pointer<vfs::file> { return {}; }
  virtual auto event(Event) -> void {}
  virtual auto log(string_view message) -> void {}
  virtual auto video(Node::Screen, const uint32_t* data, uint pitch, uint width, uint height) -> void {}
  virtual auto audio(Node::Stream) -> void {}
  virtual auto input(Node::Input) -> void {}
};

extern Platform* platform;

}

namespace ares::Core {
  auto PlatformAttach(Node::Object node) -> void { if(platform && node->name()) platform->attach(node); }
  auto PlatformDetach(Node::Object node) -> void { if(platform && node->name()) platform->detach(node); }
  auto PlatformLog(string_view text) -> void { if(platform) platform->log(text); }
}
