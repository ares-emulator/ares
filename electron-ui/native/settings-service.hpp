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
  std::string result = "{\"features\":{\"n64Gpu\":";
#if defined(VULKAN)
  result += "true";
#else
  result += "false";
#endif
  result += "},\"firmware\":[";
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

inline auto runtimeSettingsJson() -> std::string {
  std::ostringstream json;
  json.precision(17);
  json << "{\"volume\":" << settings.audio.volume << ",\"mute\":" << (settings.audio.mute ? "true" : "false")
    << ",\"luminance\":" << settings.video.luminance << ",\"saturation\":" << settings.video.saturation
    << ",\"gamma\":" << settings.video.gamma << "}";
  return json.str();
}

inline auto configureRuntime(const std::string& method, const std::vector<std::string>& arguments, bool busy) -> ControlResult {
  if(method == "runtime.settings") return arguments.empty() ? ControlResult{{}, runtimeSettingsJson()} : ControlResult{"invalid_request", {}};
  if(busy) return {"busy", {}};
  if(arguments.size() != 5 || (arguments[1] != "0" && arguments[1] != "1")) return {"invalid_request", {}};
  double values[4];
  const unsigned indices[] = {0, 2, 3, 4};
  try {
    for(unsigned i = 0; i < 4; i++) {
      size_t consumed = 0;
      values[i] = std::stod(arguments[indices[i]], &consumed);
      if(consumed != arguments[indices[i]].size() || !std::isfinite(values[i])) return {"invalid_request", {}};
    }
  } catch(...) { return {"invalid_request", {}}; }
  if(values[0] < 0 || values[0] > 1 || values[1] < 0 || values[1] > 2 || values[2] < 0 || values[2] > 2 || values[3] < 0.1 || values[3] > 3) return {"invalid_request", {}};
  settings.audio.volume = values[0]; settings.audio.mute = arguments[1] == "1";
  settings.video.luminance = values[1]; settings.video.saturation = values[2]; settings.video.gamma = values[3];
  updatePalette();
  return {{}, runtimeSettingsJson()};
}
