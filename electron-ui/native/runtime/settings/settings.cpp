#include "../runtime.hpp"

Settings settings;

auto Settings::process(bool load) -> void {
  if(load) {
    //initialize non-static default settings
    video.driver = ruby::Video::optimalDriver();
    audio.driver = "SDL";
    input.driver = "SDL";
  }

  #define bind(type, path, name) \
    if(load) { \
      if(auto node = operator[](path)) name = node.type(); \
    } else { \
      operator()(path).setValue(name); \
    } \

  bind(string,  "Video/Driver", video.driver);
  bind(string,  "Video/Monitor", video.monitor);
  bind(string,  "Video/Format", video.format);
  bind(boolean, "Video/Exclusive", video.exclusive);
  bind(boolean, "Video/Blocking", video.blocking);
  bind(boolean, "Video/PresentSRGB", video.forceSRGB);
  bind(boolean, "Video/ThreadedRenderer", video.threadedRenderer);
  bind(boolean, "Video/NativeFullScreen", video.nativeFullScreen);
  bind(boolean, "Video/Flush", video.flush);
  bind(string,  "Video/Shader", video.shader);
  bind(natural, "Video/WindowWidth", video.windowWidth);
  bind(natural, "Video/WindowHeight", video.windowHeight);
  bind(string,  "Video/Output", video.output);
  bind(natural, "Video/FixedScale", video.fixedScale);
  bind(string,  "Video/AspectCorrectionMode", video.aspectCorrection);
  bind(boolean, "Video/AdaptiveSizing", video.adaptiveSizing);
  bind(boolean, "Video/AutoCentering", video.autoCentering);
  bind(real,    "Video/Luminance", video.luminance);
  bind(real,    "Video/Saturation", video.saturation);
  bind(real,    "Video/Gamma", video.gamma);
  bind(boolean, "Video/ColorBleed", video.colorBleed);
  bind(boolean, "Video/ColorEmulation", video.colorEmulation);
  bind(boolean, "Video/InterframeBlending", video.interframeBlending);
  bind(boolean, "Video/Overscan", video.overscan);
  bind(boolean, "Video/PixelAccuracy", video.pixelAccuracy);

  bind(string,  "Audio/Device", audio.device);
  bind(natural, "Audio/Frequency", audio.frequency);
  bind(natural, "Audio/Latency", audio.latency);
  bind(boolean, "Audio/Blocking", audio.blocking);
  bind(boolean, "Audio/Dynamic", audio.dynamic);
  bind(boolean, "Audio/Mute", audio.mute);
  bind(real,    "Audio/Volume", audio.volume);
  bind(real,    "Audio/Balance", audio.balance);

  bind(string,  "Input/Defocus", input.defocus);
  bind(string,  "Input/DigitalToAnalog", input.digitalToAnalog);
  bind(natural, "Input/DigitalToAnalogTime", input.digitalToAnalogTime);
  input.digitalToAnalogTime = max(100u, min(1000u, input.digitalToAnalogTime));

  bind(boolean, "Boot/Fast", boot.fast);
  bind(boolean, "Boot/Debugger", boot.debugger);
  bind(boolean, "Boot/AwaitGDBClient", boot.awaitGDBClient);
  bind(string,  "Boot/Prefer", boot.prefer);

  bind(boolean, "General/ShowStatusBar", general.showStatusBar);
  bind(boolean, "General/Rewind", general.rewind);
  bind(boolean, "General/RunAhead", general.runAhead);
  bind(boolean, "General/AutoSaveMemory", general.autoSaveMemory);
  bind(boolean, "General/NoFilePrompt", general.noFilePrompt);

  bind(natural, "Rewind/Length", rewind.length);
  bind(natural, "Rewind/Frequency", rewind.frequency);
  bind(boolean, "Rewind/Mute", rewind.mute);

  bind(string,  "Paths/Home", paths.home);
  bind(string,  "Paths/Firmware", paths.firmware);
  bind(string,  "Paths/Saves", paths.saves);
  bind(string,  "Paths/Screenshots", paths.screenshots);
  bind(string,  "Paths/Debugging", paths.debugging);
  bind(string,  "Paths/ArcadeRoms", paths.arcadeRoms);
  bind(string,  "Paths/SuperFamicom/GameBoy", paths.superFamicom.gameBoy);
  bind(string,  "Paths/SuperFamicom/BSMemory", paths.superFamicom.bsMemory);
  bind(string,  "Paths/SuperFamicom/SufamiTurbo", paths.superFamicom.sufamiTurbo);

  bind(natural, "Developer/DebugServerPort", developer.debugServerPort);
  bind(boolean, "Developer/DebugServerEnabled", developer.debugServerEnabled);
  bind(boolean, "Developer/DebugServerUseIPv4", developer.debugServerUseIPv4);
  bind(boolean, "Developer/HomebrewMode", developer.homebrewMode);
  bind(boolean, "Developer/DeterministicEntropy", developer.deterministicEntropy);
  bind(boolean, "Developer/ForceInterpreter", developer.forceInterpreter);

  bind(boolean, "Nintendo64/ExpansionPak", nintendo64.expansionPak);
  bind(string,  "Nintendo64/ControllerPakBankString", nintendo64.controllerPakBankString);
  bind(string,  "Nintendo64/Quality", nintendo64.quality);
  bind(boolean, "Nintendo64/Supersampling", nintendo64.supersampling);
  bind(boolean, "Nintendo64/DisableVideoInterfaceProcessing", nintendo64.disableVideoInterfaceProcessing);
  bind(boolean, "Nintendo64/WeaveDeinterlacing", nintendo64.weaveDeinterlacing);

  bind(boolean, "GameBoyAdvance/Player", gameBoyAdvance.player);

  bind(boolean, "SuperFamicom/DeepBlackBoost", superFamicom.deepBlackBoost);

  bind(boolean, "MegaDrive/TMSS", megadrive.tmss);

  for(u32 index : range(9)) {
    string name = {"Recent/Game-", 1 + index};
    bind(string, name, recent.game[index]);
  }

  for(u32 index : range(5)) {
    auto& port = virtualPorts[index];
    for(auto& input : port.pad.inputs) {
      string name = {"VirtualPad", 1 + index, "/", string{input.name}.replace(" ", ".").replace("(", ".").replace(")", "")}, value;
      if(load == 0) for(auto& assignment : input.mapping->assignments) value.append(assignment, ";");
      if(load == 0) value.trimRight(";", 1L);
      bind(string, name, value);
      if(load == 1) {
        auto parts = nall::split(value, ";");
        parts.resize(BindingLimit);
        for(u32 binding : range(BindingLimit)) input.mapping->assignments[binding] = parts[binding];
      }
    }
    for(auto& input : port.mouse.inputs) {
      string name = {"VirtualMouse", 1 + index, "/", input.name}, value;
      if(load == 0) for(auto& assignment : input.mapping->assignments) value.append(assignment, ";");
      if(load == 0) value.trimRight(";", 1L);
      bind(string, name, value);
      if(load == 1) {
        auto parts = nall::split(value, ";");
        parts.resize(BindingLimit);
        for(u32 binding : range(BindingLimit)) input.mapping->assignments[binding] = parts[binding];
      }
    }
  }

  for(auto& emulator : emulators) {
    string base = string{emulator->name}.replace(" ", "");
    base.replace("(", "").replace(")", "");
    for(auto& port : emulator->ports) {
      for(auto& device : port.devices) {
        if(!device.hasDirectMappings()) continue;
        string portName = string{port.name}.replace(" ", ".").replace("[", "").replace("]", "").replace("(", "").replace(")", "").replace("*", "Star").replace("#", "Pound");
        string deviceName = string{device.name}.replace(" ", ".").replace("[", "").replace("]", "").replace("(", "").replace(")", "").replace("*", "Star").replace("#", "Pound");
        for(auto& input : device.inputs) {
          auto& mapping = input.configuredMapping();
          string inputName = string{input.name}.replace(" ", ".").replace("[", "").replace("]", "").replace("(", ".").replace(")", "").replace("*", "Star").replace("#", "Pound");
          string name = {base, "/Input/", portName, "/", deviceName, "/", inputName}, value;
          if(load == 0) for(auto& assignment : mapping.assignments) value.append(assignment, ";");
          if(load == 0) value.trimRight(";", 1L);
          bind(string, name, value);
          if(load == 1) {
            auto parts = nall::split(value, ";");
            parts.resize(BindingLimit);
            for(u32 binding : range(BindingLimit)) mapping.assignments[binding] = parts[binding];
          }
        }
        for(auto& pair : device.pairs) {
          string pairName = string{pair.name}.replace(" ", ".").replace("[", "").replace("]", "").replace("(", ".").replace(")", "").replace("*", "Star").replace("#", "Pound");
          for(auto index : range(2)) {
            string suffix = index == 0 ? "Lo" : "Hi";
            auto& mapping = index == 0 ? pair.configuredMappingLo() : pair.configuredMappingHi();
            string name = {base, "/Input/", portName, "/", deviceName, "/", pairName, "/", suffix}, value;
            if(load == 0) for(auto& assignment : mapping.assignments) value.append(assignment, ";");
            if(load == 0) value.trimRight(";", 1L);
            bind(string, name, value);
            if(load == 1) {
              auto parts = nall::split(value, ";");
              parts.resize(BindingLimit);
              for(u32 binding : range(BindingLimit)) mapping.assignments[binding] = parts[binding];
            }
          }
        }
      }
    }
  }

  for(auto& emulator : emulators) {
    string base = string{emulator->name}.replace(" ", ""), name;
    base.replace("(", "").replace(")", "");
    name = {base, "/Visible"};
    bind(boolean, name, emulator->configuration.visible);
    name = {base, "/Path"};
    bind(string,  name, emulator->configuration.game);

    emulator->configuration.peripherals.resize(emulator->ports.size());
    for(auto index : range(emulator->ports.size())) {
      string portName = string{emulator->ports[index].name}
        .replace(" ", "").replace("[", "").replace("]", "").replace("(", "").replace(")", "")
        .replace("*", "Star").replace("#", "Pound").replace("/", "");
      name = {base, "/", portName};
      bind(string, name, emulator->configuration.peripherals[index]);
    }

    for(auto& firmware : emulator->firmware) {
      string name = firmwareSettingPath(emulator->name, firmware.type, firmware.region);
      bind(string, name, firmware.location);
    }
  }

  #undef bind
}
