#pragma once

namespace ares::Arcade {

struct Dip {
  struct Option {
    string name;
    u32 value;
  };

  struct Setting {
    Node::Setting::String node;
    u32 port;
    u32 mask;
    std::vector<Option> options;
  };

  struct Port {
    string name;
    u32 value = 0xffff'ffff;
  };

  auto load(Node::Object parent, VFS::Pak pak, std::initializer_list<string> names, u32 excluded = 0, bool reuseNode = false, string manifest = "manifest.bml") -> void;
  auto unload() -> void;
  auto read(string name) const -> u32;
  auto update(u32 index, string value) -> void;
  auto persist() -> void;

  Node::Object node;
  VFS::Pak pak;
  bool ownsNode = false;
  std::vector<Port> ports;
  std::vector<Setting> settings;
};

}
