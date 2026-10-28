#include <ares/ares.hpp>
#include "dip-switches.hpp"

namespace ares::Arcade {

auto Dip::load(Node::Object parent, VFS::Pak pak, std::initializer_list<string> names, u32 excluded, bool reuseNode, string manifest) -> void {
  unload();
  for(auto name : names) ports.push_back({name});
  if(!pak) return;
  this->pak = pak;

  Markup::Node saved;
  if(auto state = pak->read("dips.bml")) saved = BML::unserialize(state->reads());

  auto fp = pak->read(manifest);
  if(!fp) return;
  auto document = BML::unserialize(fp->reads());
  auto games = document.find("game");
  if(games.size() != 1) return;

  ownsNode = !reuseNode;
  node = reuseNode ? parent : parent->append<Node::Object>("DIP Switches");
  for(auto dip : games.front()) {
    if(dip.name() != "dip") continue;
    auto portName = dip["port"].string();
    u32 port = 0;
    while(port < ports.size() && ports[port].name != portName) port++;
    if(port == ports.size()) continue;

    auto maskValue = dip["mask"].natural();
    auto defaultValue = dip["default"].natural();
    if(!maskValue || maskValue > 0xffff'ffff || (defaultValue & ~maskValue) || (maskValue & excluded)) continue;
    u32 mask = maskValue;
    ports[port].value = (ports[port].value & ~mask) | defaultValue;

    auto name = dip["name"].string();
    if(!name || name == "Unused" || name == "Unknown") continue;

    std::vector<Option> options;
    string defaultName;
    for(auto option : dip) {
      if(option.name() != "option") continue;
      auto optionName = option["name"].string();
      auto optionValue = option["value"].natural();
      if(!optionName || optionValue > 0xffff'ffff || (optionValue & ~maskValue)) continue;
      for(auto& existing : options) {
        if(existing.name == optionName) {
          optionName.append(" (", optionValue, ")");
          break;
        }
      }
      if(optionValue == defaultValue) defaultName = optionName;
      options.push_back({optionName, (u32)optionValue});
    }
    if(options.empty() || !defaultName) continue;

    for(auto stored : saved.find("dip")) {
      if(stored["port"].string() != portName || stored["mask"].natural() != mask) continue;
      auto value = stored["value"].natural();
      for(auto& option : options) {
        if(option.value != value) continue;
        defaultName = option.name;
        ports[port].value = (ports[port].value & ~mask) | option.value;
        break;
      }
      break;
    }

    auto index = settings.size();
    settings.push_back({{}, port, mask, std::move(options)});
    auto& setting = settings.back();
    std::vector<string> allowed;
    for(auto& option : setting.options) allowed.push_back(option.name);
    setting.node = node->append<Node::Setting::String>(name, defaultName, [this, index](auto value) {
      update(index, value);
    });
    setting.node->setAllowedValues(std::move(allowed));
    setting.node->setDynamic(true);
  }
}

auto Dip::unload() -> void {
  if(ownsNode) {
    if(auto parent = Node::parent(node)) parent->remove(node);
  }
  node.reset();
  pak.reset();
  ownsNode = false;
  settings.clear();
  ports.clear();
}

auto Dip::read(string name) const -> u32 {
  for(auto& port : ports) {
    if(port.name == name) return port.value;
  }
  return 0xffff'ffff;
}

auto Dip::update(u32 index, string value) -> void {
  if(index >= settings.size()) return;
  auto& setting = settings[index];
  for(auto& option : setting.options) {
    if(option.name != value) continue;
    auto& port = ports[setting.port];
    port.value = (port.value & ~setting.mask) | option.value;
    persist();
    return;
  }
}

auto Dip::persist() -> void {
  if(!pak) return;
  auto fp = pak->write("dips.bml");
  if(!fp) return;
  string state;
  for(auto& setting : settings) {
    auto& port = ports[setting.port];
    state.append("dip\n");
    state.append("  port:  ", port.name, "\n");
    state.append("  mask:  ", setting.mask, "\n");
    state.append("  value: ", port.value & setting.mask, "\n");
  }
  fp->resize(state.size());
  fp->seek(0);
  fp->writes(state);
}

}
