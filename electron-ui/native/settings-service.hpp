#pragma once

inline auto visitSettings(const Markup::Node& node, const string& prefix,
    const std::function<void(const string&, const string&)>& visit) -> void {
  for(auto child : node) {
    string key = prefix ? string{prefix, "/", child.name()} : child.name();
    if(child.size()) visitSettings(child, key, visit);
    else visit(key, child.value());
  }
}

inline auto settingsInfo() -> std::string {
  std::string result = "{\"firmware\":[";
  bool first = true;
  for(auto& entry : emulators) for(auto& firmware : entry->firmware) {
    if(!first) result += ',';
    first = false;
    auto key = firmwareSettingPath(entry->name, firmware.type, firmware.region);
    result += "{\"system\":" + controlQuote(entry->name.data())
      + ",\"type\":" + controlQuote(firmware.type.data())
      + ",\"region\":" + controlQuote(firmware.region.data())
      + ",\"key\":" + controlQuote(key.data()) + "}";
  }
  result += "],\"imported\":{";
  first = true;
  visitSettings(settings, {}, [&](const string& key, const string& value) {
    if(!first) result += ',';
    first = false;
    result += controlQuote(key.data()) + ':' + controlQuote(value.data());
  });
  return result + "}}";
}
