static auto parseDipByte(string text, u8& value) -> bool {
  if(!text) return false;

  u32 base = 10;
  u32 offset = 0;
  if(text.size() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
    base = 16;
    offset = 2;
  }
  if(offset == text.size()) return false;

  u32 result = 0;
  for(auto index : range(offset, text.size())) {
    u32 digit;
    auto character = text[(u32)index];
    if(character >= '0' && character <= '9') digit = character - '0';
    else if(character >= 'a' && character <= 'f') digit = character - 'a' + 10;
    else if(character >= 'A' && character <= 'F') digit = character - 'A' + 10;
    else return false;
    if(digit >= base || result > (255 - digit) / base) return false;
    result = result * base + digit;
  }

  value = result;
  return true;
}

auto VsUniSystem::DIPSwitches::connect() -> bool {
  if(!cartridge.pak) return false;

  auto manifestFile = cartridge.pak->read("manifest.bml");
  if(!manifestFile) return false;
  auto document = BML::unserialize(manifestFile->reads());
  auto games = document.find("game");
  if(games.size() != 1) return false;

  u8 usedMasks = 0;
  std::vector<Definition> definitions;
  for(auto node : games.front()) {
    if(node.name() != "dip") continue;

    Definition definition;
    definition.name = node["name"].string();
    if(!definition.name) return false;
    if(node["port"].string() != "DSW0") return false;
    if(!parseDipByte(node["mask"].string(), definition.mask) || !definition.mask) return false;
    if(usedMasks & definition.mask) return false;
    if(!parseDipByte(node["default"].string(), definition.defaultValue)) return false;
    if(definition.defaultValue & ~definition.mask) return false;

    bool hasDefault = false;
    for(auto optionNode : node) {
      if(optionNode.name() != "option") continue;

      Option option;
      option.name = optionNode["name"].string();
      if(!option.name) return false;
      if(!parseDipByte(optionNode["value"].string(), option.value)) return false;
      if(option.value & ~definition.mask) return false;
      for(auto& existing : definition.options) {
        if(existing.name == option.name) return false;
      }
      if(option.value == definition.defaultValue) hasDefault = true;
      definition.options.push_back(std::move(option));
    }

    if(definition.options.empty() || !hasDefault) return false;
    usedMasks |= definition.mask;
    definitions.push_back(std::move(definition));
  }

  load(std::move(definitions));
  return true;
}

auto VsUniSystem::DIPSwitches::load(std::vector<Definition> definitions) -> void {
  node = vsUniSystem.parent->append<Node::Object>("DIP Switches");
  settings.reserve(definitions.size());

  for(auto& definition : definitions) {
    value |= definition.defaultValue;
    auto index = settings.size();

    std::vector<string> allowedValues;
    for(auto& option : definition.options) allowedValues.push_back(option.name);
    string defaultValue;
    for(auto& option : definition.options) {
      if(option.value == definition.defaultValue) defaultValue = option.name;
    }

    settings.push_back({{}, definition.mask, std::move(definition.options)});
    auto& setting = settings.back();
    setting.node = node->append<Node::Setting::String>(definition.name, defaultValue, [index](auto value) {
      vsUniSystem.dipSwitches.update(index, value);
    });
    setting.node->setAllowedValues(std::move(allowedValues));
  }
}

auto VsUniSystem::DIPSwitches::disconnect() -> void {
  if(vsUniSystem.parent && node) vsUniSystem.parent->remove(node);
  node.reset();
  settings.clear();
  value = 0;
}

auto VsUniSystem::DIPSwitches::update(u32 index, string value) -> void {
  if(index >= settings.size()) return;
  auto& setting = settings[index];
  for(auto& option : setting.options) {
    if(option.name != value) continue;
    this->value = (this->value & ~setting.mask) | (option.value & setting.mask);
    return;
  }
}
