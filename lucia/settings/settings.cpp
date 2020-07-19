#include "../lucia.hpp"
#include "video.cpp"
#include "audio.cpp"
#include "input.cpp"
#include "hotkeys.cpp"
#include "emulators.cpp"
#include "options.cpp"
#include "firmware.cpp"
#include "paths.cpp"
#include "drivers.cpp"
#include "home.cpp"

Settings settings;
namespace Instances { Instance<SettingsWindow> settingsWindow; }
SettingsWindow& settingsWindow = Instances::settingsWindow();
VideoSettings& videoSettings = settingsWindow.videoSettings;
AudioSettings& audioSettings = settingsWindow.audioSettings;
InputSettings& inputSettings = settingsWindow.inputSettings;
HotkeySettings& hotkeySettings = settingsWindow.hotkeySettings;
EmulatorSettings& emulatorSettings = settingsWindow.emulatorSettings;
OptionSettings& optionSettings = settingsWindow.optionSettings;
FirmwareSettings& firmwareSettings = settingsWindow.firmwareSettings;
PathSettings& pathSettings = settingsWindow.pathSettings;
DriverSettings& driverSettings = settingsWindow.driverSettings;

auto Settings::load() -> void {
  Markup::Node::operator=(BML::unserialize(string::read(locate("settings.bml")), " "));
  process(true);
  save();
}

auto Settings::save() -> void {
  process(false);
  file::write(locate("settings.bml"), BML::serialize(*this, " "));
}

auto Settings::process(bool load) -> void {
  if(load) {
    //initialize non-static default settings
    video.driver = ruby::Video::optimalDriver();
    audio.driver = ruby::Audio::optimalDriver();
    input.driver = ruby::Input::optimalDriver();
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
  bind(boolean, "Video/Flush", video.flush);
  bind(string,  "Video/Shader", video.shader);
  bind(natural, "Video/Multiplier", video.multiplier);
  bind(string,  "Video/Output", video.output);
  bind(boolean, "Video/AspectCorrection", video.aspectCorrection);
  bind(boolean, "Video/AdaptiveSizing", video.adaptiveSizing);
  bind(boolean, "Video/AutoCentering", video.autoCentering);
  bind(real,    "Video/Luminance", video.luminance);
  bind(real,    "Video/Saturation", video.saturation);
  bind(real,    "Video/Gamma", video.gamma);
  bind(boolean, "Video/ColorBleed", video.colorBleed);
  bind(boolean, "Video/ColorEmulation", video.colorEmulation);
  bind(boolean, "Video/InterframeBlending", video.interframeBlending);
  bind(boolean, "Video/Overscan", video.overscan);

  bind(string,  "Audio/Driver", audio.driver);
  bind(string,  "Audio/Device", audio.device);
  bind(natural, "Audio/Frequency", audio.frequency);
  bind(natural, "Audio/Latency", audio.latency);
  bind(boolean, "Audio/Exclusive", audio.exclusive);
  bind(boolean, "Audio/Blocking", audio.blocking);
  bind(boolean, "Audio/Dynamic", audio.dynamic);
  bind(boolean, "Audio/Mute", audio.mute);
  bind(real,    "Audio/Volume", audio.volume);
  bind(real,    "Audio/Balance", audio.balance);

  bind(string,  "Input/Driver", input.driver);
  bind(string,  "Input/Defocus", input.defocus);

  bind(boolean, "General/ShowStatusBar", general.showStatusBar);
  bind(boolean, "General/Rewind", general.rewind);
  bind(boolean, "General/RunAhead", general.runAhead);
  bind(boolean, "General/AutoSaveMemory", general.autoSaveMemory);
  bind(boolean, "General/FastBoot", general.fastBoot);
  bind(boolean, "General/AutoDebug", general.autoDebug);
  bind(boolean, "General/NativeFileDialogs", general.nativeFileDialogs);
  bind(boolean, "General/GroupEmulators", general.groupEmulators);

  bind(natural, "Rewind/Length", rewind.length);
  bind(natural, "Rewind/Frequency", rewind.frequency);

  bind(string,  "Paths/Saves", paths.saves);
  bind(string,  "Paths/Patches", paths.patches);
  bind(string,  "Paths/Screenshots", paths.screenshots);
  bind(string,  "Paths/Debugging", paths.debugging);
  bind(string,  "Paths/Firmware", paths.firmware);

  for(uint index : range(9)) {
    string name = {"Recent/Game-", 1 + index};
    bind(string, name, recent.game[index]);
  }

  for(auto& mapping : virtualPad.mappings) {
    string name = {"VirtualPad/", mapping->name}, value;
    if(load == 0) for(auto& assignment : mapping->assignments) value.append(assignment, ";");
    if(load == 0) value.trimRight(";", 1L);
    bind(string, name, value);
    if(load == 1) for(uint binding : range(BindingLimit)) mapping->assignments[binding] = value.split(";")(binding);
  }

  for(auto& mapping : inputManager.hotkeys) {
    string name = {"Hotkey/", string{mapping.name}.replace(" ", "")}, value;
    if(load == 0) for(auto& assignment : mapping.assignments) value.append(assignment, ";");
    if(load == 0) value.trimRight(";", 1L);
    bind(string, name, value);
    if(load == 1) for(uint binding : range(BindingLimit)) mapping.assignments[binding] = value.split(";")(binding);
  }

  for(auto& emulator : emulators) {
    string base = string{emulator->name}.replace(" ", ""), name;
    name = {base, "/Visible"};
    bind(boolean, name, emulator->configuration.visible);
    name = {base, "/Path"};
    bind(string,  name, emulator->configuration.game);
    for(auto& firmware : emulator->firmware) {
      string name = {base, "/Firmware/", firmware.type, ".", firmware.region};
      bind(string, name, firmware.location);
    }
  }

  #undef bind
}

//

SettingsWindow::SettingsWindow() {
  onClose([&] {
    setVisible(false);
    //cancel any pending input assignment requests, if any
    inputSettings.setVisible(false);
    hotkeySettings.setVisible(false);
  });

  layout.setPadding(5_sx, 5_sy);

  panelList.append(ListViewItem().setText("Video").setIcon(Icon::Device::Display));
  panelList.append(ListViewItem().setText("Audio").setIcon(Icon::Device::Speaker));
  panelList.append(ListViewItem().setText("Input").setIcon(Icon::Device::Joypad));
  panelList.append(ListViewItem().setText("Hotkeys").setIcon(Icon::Device::Keyboard));
  panelList.append(ListViewItem().setText("Emulators").setIcon(Icon::Place::Server));
  panelList.append(ListViewItem().setText("Options").setIcon(Icon::Action::Settings));
  panelList.append(ListViewItem().setText("Firmware").setIcon(Icon::Emblem::Binary));
  panelList.append(ListViewItem().setText("Paths").setIcon(Icon::Emblem::Folder));
  panelList.append(ListViewItem().setText("Drivers").setIcon(Icon::Place::Settings));
  panelList.onChange([&] { eventChange(); });

  panelContainer.append(videoSettings, Size{~0, ~0});
  panelContainer.append(audioSettings, Size{~0, ~0});
  panelContainer.append(inputSettings, Size{~0, ~0});
  panelContainer.append(hotkeySettings, Size{~0, ~0});
  panelContainer.append(emulatorSettings, Size{~0, ~0});
  panelContainer.append(optionSettings, Size{~0, ~0});
  panelContainer.append(firmwareSettings, Size{~0, ~0});
  panelContainer.append(pathSettings, Size{~0, ~0});
  panelContainer.append(driverSettings, Size{~0, ~0});
  panelContainer.append(homePanel, Size{~0, ~0});

  videoSettings.construct();
  audioSettings.construct();
  inputSettings.construct();
  hotkeySettings.construct();
  emulatorSettings.construct();
  optionSettings.construct();
  firmwareSettings.construct();
  pathSettings.construct();
  driverSettings.construct();
  homePanel.construct();

  setDismissable();
  setTitle("Configuration");
  setSize({690_sx, 405_sy});
  setAlignment({0.0, 1.0});
}

auto SettingsWindow::show(const string& panel) -> void {
  for(auto item : panelList.items()) {
    if(item.text() == panel) {
      item.setSelected();
      eventChange();
      break;
    }
  }
  setVisible();
  setFocused();
  panelList.setFocused();
}

auto SettingsWindow::eventChange() -> void {
  videoSettings.setVisible(false);
  audioSettings.setVisible(false);
  inputSettings.setVisible(false);
  hotkeySettings.setVisible(false);
  emulatorSettings.setVisible(false);
  optionSettings.setVisible(false);
  firmwareSettings.setVisible(false);
  pathSettings.setVisible(false);
  driverSettings.setVisible(false);
  homePanel.setVisible(false);

  bool found = false;
  if(auto item = panelList.selected()) {
    if(item.text() == "Video"    ) found = true, videoSettings.setVisible();
    if(item.text() == "Audio"    ) found = true, audioSettings.setVisible();
    if(item.text() == "Input"    ) found = true, inputSettings.setVisible();
    if(item.text() == "Hotkeys"  ) found = true, hotkeySettings.setVisible();
    if(item.text() == "Emulators") found = true, emulatorSettings.setVisible();
    if(item.text() == "Options"  ) found = true, optionSettings.setVisible();
    if(item.text() == "Firmware" ) found = true, firmwareSettings.setVisible();
    if(item.text() == "Paths"    ) found = true, pathSettings.setVisible();
    if(item.text() == "Drivers"  ) found = true, driverSettings.setVisible();
  }
  if(!found) homePanel.setVisible();

  panelContainer.resize();
}
