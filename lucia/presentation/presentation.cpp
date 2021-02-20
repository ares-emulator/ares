#include "../lucia.hpp"
namespace Instances { Instance<Presentation> presentation; }
Presentation& presentation = Instances::presentation();

Presentation::Presentation() {
  loadMenu.setText("Load");

  systemMenu.setVisible(false);

  settingsMenu.setText("Settings");
  videoSizeMenu.setText("Size").setIcon(Icon::Emblem::Image);
  //determine the largest multiplier that can be used by the largest monitor found
  u32 monitorHeight = 1;
  for(u32 monitor : range(Monitor::count())) {
    monitorHeight = max(monitorHeight, Monitor::workspace(monitor).height());
  }
  //generate size menu
  u32 multipliers = max(1, monitorHeight / 240);
  for(u32 multiplier : range(2, multipliers + 1)) {
    MenuItem item{&videoSizeMenu};
    item.setText({multiplier, "x (", 240 * multiplier, "p)"});
    item.onActivate([=] {
      settings.video.multiplier = multiplier;
      resizeWindow();
    });
  }
  videoSizeMenu.append(MenuSeparator());
  MenuItem centerWindow{&videoSizeMenu};
  centerWindow.setText("Center Window").setIcon(Icon::Place::Settings).onActivate([&] {
    setAlignment(Alignment::Center);
  });
  videoOutputMenu.setText("Output").setIcon(Icon::Emblem::Image);
  videoOutputCenter.setText("Center").onActivate([&] {
    settings.video.output = "Center";
  });
  videoOutputScale.setText("Scale").onActivate([&] {
    settings.video.output = "Scale";
  });
  videoOutputStretch.setText("Stretch").onActivate([&] {
    settings.video.output = "Stretch";
  });
  if(settings.video.output == "Center") videoOutputCenter.setChecked();
  if(settings.video.output == "Scale") videoOutputScale.setChecked();
  if(settings.video.output == "Stretch") videoOutputStretch.setChecked();
  videoAspectCorrection.setText("Aspect Correction").setChecked(settings.video.aspectCorrection).onToggle([&] {
    settings.video.aspectCorrection = videoAspectCorrection.checked();
    if(settings.video.adaptiveSizing) resizeWindow();
  });
  videoAdaptiveSizing.setText("Adaptive Sizing").setChecked(settings.video.adaptiveSizing).onToggle([&] {
    if(settings.video.adaptiveSizing = videoAdaptiveSizing.checked()) resizeWindow();
  });
  videoAutoCentering.setText("Auto Centering").setChecked(settings.video.autoCentering).onToggle([&] {
    if(settings.video.autoCentering = videoAutoCentering.checked()) resizeWindow();
  });
  videoShaderMenu.setText("Shader").setIcon(Icon::Emblem::Image);
  loadShaders();
  bootOptionsMenu.setText("Boot Options").setIcon(Icon::Place::Settings);
  fastBoot.setText("Fast Boot").setChecked(settings.boot.fast).onToggle([&] {
    settings.boot.fast = fastBoot.checked();
  });
  launchDebugger.setText("Launch Debugger").setChecked(settings.boot.debugger).onToggle([&] {
    settings.boot.debugger = launchDebugger.checked();
  });
  preferNTSCU.setText("Prefer US").onActivate([&] {
    settings.boot.prefer = "NTSC-U";
  });
  preferNTSCJ.setText("Prefer Japan").onActivate([&] {
    settings.boot.prefer = "NTSC-J";
  });
  preferPAL.setText("Prefer Europe").onActivate([&] {
    settings.boot.prefer = "PAL";
  });
  if(settings.boot.prefer == "NTSC-U") preferNTSCU.setChecked();
  if(settings.boot.prefer == "NTSC-J") preferNTSCJ.setChecked();
  if(settings.boot.prefer == "PAL") preferPAL.setChecked();
  muteAudioSetting.setText("Mute Audio").setChecked(settings.audio.mute).onToggle([&] {
    settings.audio.mute = muteAudioSetting.checked();
  });
  showStatusBarSetting.setText("Show Status Bar").setChecked(settings.general.showStatusBar).onToggle([&] {
    settings.general.showStatusBar = showStatusBarSetting.checked();
    if(!showStatusBarSetting.checked()) {
      layout.remove(statusLayout);
    } else {
      layout.append(statusLayout, Size{~0, StatusHeight});
    }
    if(visible()) resizeWindow();
  }).doToggle();
  videoSettingsAction.setText("Video ...").setIcon(Icon::Device::Display).onActivate([&] {
    settingsWindow.show("Video");
  });
  audioSettingsAction.setText("Audio ...").setIcon(Icon::Device::Speaker).onActivate([&] {
    settingsWindow.show("Audio");
  });
  inputSettingsAction.setText("Input ...").setIcon(Icon::Device::Joypad).onActivate([&] {
    settingsWindow.show("Input");
  });
  hotkeySettingsAction.setText("Hotkeys ...").setIcon(Icon::Device::Keyboard).onActivate([&] {
    settingsWindow.show("Hotkeys");
  });
  emulatorSettingsAction.setText("Emulators ...").setIcon(Icon::Place::Server).onActivate([&] {
    settingsWindow.show("Emulators");
  });
  optionSettingsAction.setText("Options ...").setIcon(Icon::Action::Settings).onActivate([&] {
    settingsWindow.show("Options");
  });
  firmwareSettingsAction.setText("Firmware ...").setIcon(Icon::Emblem::Binary).onActivate([&] {
    settingsWindow.show("Firmware");
  });
  pathSettingsAction.setText("Paths ...").setIcon(Icon::Emblem::Folder).onActivate([&] {
    settingsWindow.show("Paths");
  });
  driverSettingsAction.setText("Drivers ...").setIcon(Icon::Place::Settings).onActivate([&] {
    settingsWindow.show("Drivers");
  });

  toolsMenu.setVisible(false).setText("Tools");
  saveStateMenu.setText("Save State").setIcon(Icon::Media::Record);
  for(u32 slot : range(9)) {
    MenuItem item{&saveStateMenu};
    item.setText({"Slot ", 1 + slot}).onActivate([=] {
      program.stateSave(1 + slot);
    });
  }
  loadStateMenu.setText("Load State").setIcon(Icon::Media::Rewind);
  for(u32 slot : range(9)) {
    MenuItem item{&loadStateMenu};
    item.setText({"Slot ", 1 + slot}).onActivate([=] {
      program.stateLoad(1 + slot);
    });
  }
  captureScreenshot.setText("Capture Screenshot").setIcon(Icon::Emblem::Image).onActivate([&] {
    program.requestScreenshot = true;
  });
  pauseEmulation.setText("Pause Emulation").onToggle([&] {
    program.pause(!program.paused);
  });
  manifestViewerAction.setText("Manifest").setIcon(Icon::Emblem::Binary).onActivate([&] {
    toolsWindow.show("Manifest");
  });
  memoryEditorAction.setText("Memory").setIcon(Icon::Device::Storage).onActivate([&] {
    toolsWindow.show("Memory");
  });
  graphicsViewerAction.setText("Graphics").setIcon(Icon::Emblem::Image).onActivate([&] {
    toolsWindow.show("Graphics");
  });
  streamManagerAction.setText("Streams").setIcon(Icon::Emblem::Audio).onActivate([&] {
    toolsWindow.show("Streams");
  });
  propertiesViewerAction.setText("Properties").setIcon(Icon::Emblem::Text).onActivate([&] {
    toolsWindow.show("Properties");
  });
  traceLoggerAction.setText("Tracer").setIcon(Icon::Emblem::Script).onActivate([&] {
    toolsWindow.show("Tracer");
  });

  helpMenu.setText("Help");
  aboutAction.setText("About ...").setIcon(Icon::Prompt::Question).onActivate([&] {
    image logo{Resource::Ares::Logo};
    logo.shrink();
    AboutDialog()
    .setLogo(logo)
    .setDescription({ares::Name, "/lucia — a simplified multi-system emulator"})
    .setVersion(ares::Version)
    .setCopyright(ares::Copyright)
    .setLicense(ares::License, ares::LicenseURI)
    .setWebsite(ares::Website, ares::WebsiteURI)
    .setAlignment(presentation)
    .show();
  });

  viewport.setDroppable().onDrop([&](auto filenames) {
    if(filenames.size() != 1) return;
    if(auto emulator = program.identify(filenames.first())) {
      program.load(emulator, filenames.first());
    }
  });

  iconLayout.setCollapsible();

  iconSpacer.setCollapsible().setColor({0, 0, 0}).setDroppable().onDrop([&](auto filenames) {
    viewport.doDrop(filenames);
  });

  image icon{Resource::Ares::Icon};
  icon.alphaBlend(0x000000);
  iconCanvas.setCollapsible().setIcon(icon).setDroppable().onDrop([&](auto filenames) {
    viewport.doDrop(filenames);
  });

  iconBottom.setCollapsible().setColor({0, 0, 0}).setDroppable().onDrop([&](auto filenames) {
    viewport.doDrop(filenames);
  });

  spacerLeft .setBackgroundColor({32, 32, 32});
  statusLeft .setBackgroundColor({32, 32, 32}).setForegroundColor({255, 255, 255});
  statusRight.setBackgroundColor({32, 32, 32}).setForegroundColor({255, 255, 255});
  spacerRight.setBackgroundColor({32, 32, 32});

  statusLeft .setAlignment(0.0).setFont(Font().setBold());
  statusRight.setAlignment(1.0).setFont(Font().setBold());

  onClose([&] {
    program.quit();
  });

  resizeWindow();
  setTitle({ares::Name, " v", ares::Version});
  setBackgroundColor({0, 0, 0});
  setAlignment(Alignment::Center);
  setVisible();

  #if defined(PLATFORM_MACOS)
  Application::Cocoa::onAbout([&] { aboutAction.doActivate(); });
  Application::Cocoa::onActivate([&] { setFocused(); });
  Application::Cocoa::onPreferences([&] { settingsWindow.show("Video"); });
  Application::Cocoa::onQuit([&] { doClose(); });
  #endif
}

auto Presentation::resizeWindow() -> void {
  if(fullScreen()) return;
  if(maximized()) setMaximized(false);

  u32 multiplier = max(2, settings.video.multiplier);
  u32 viewportWidth = 320 * multiplier;
  u32 viewportHeight = 240 * multiplier;

  if(emulator && program.screens) {
    auto& node = program.screens.first();
    u32 videoWidth = node->width() * node->scaleX();
    u32 videoHeight = node->height() * node->scaleY();
    if(settings.video.aspectCorrection) videoWidth = videoWidth * node->aspectX() / node->aspectY();
    if(node->rotation() == 90 || node->rotation() == 270) swap(videoWidth, videoHeight);

    u32 multiplierX = viewportWidth / videoWidth;
    u32 multiplierY = viewportHeight / videoHeight;
    u32 multiplier = min(multiplierX, multiplierY);

    viewportWidth = videoWidth * multiplier;
    viewportHeight = videoHeight * multiplier;
  }

  u32 statusHeight = showStatusBarSetting.checked() ? StatusHeight : 0;

  if(settings.video.autoCentering) {
    setGeometry(Alignment::Center, {viewportWidth, viewportHeight + statusHeight});
  } else {
    setSize({viewportWidth, viewportHeight + statusHeight});
  }

  setMinimumSize({256, 240 + statusHeight});
}

auto Presentation::loadEmulators() -> void {
  loadMenu.reset();

  //clean up the recent games history first
  vector<string> recentGames;
  for(u32 index : range(9)) {
    auto entry = settings.recent.game[index];
    auto system = entry.split(";", 1L)(0);
    auto location = entry.split(";", 1L)(1);
    if(file::exists(location)) {  //remove missing files
      if(!recentGames.find(entry)) {  //remove duplicate entries
        recentGames.append(entry);
      }
    }
    settings.recent.game[index] = {};
  }

  //build recent games list
  u32 count = 0;
  for(auto& game : recentGames) {
    settings.recent.game[count++] = game;
  }
  { Menu recentGames{&loadMenu};
    recentGames.setIcon(Icon::Action::Open);
    recentGames.setText("Recent Games");
    for(u32 index : range(count)) {
      MenuItem item{&recentGames};
      auto entry = settings.recent.game[index];
      auto system = entry.split(";", 1L)(0);
      auto location = entry.split(";", 1L)(1);
      item.setIcon(Icon::Emblem::File);
      item.setText(Location::prefix(location));
      item.onActivate([=] {
        for(auto& emulator : emulators) {
          if(emulator->name == system) {
            return (void)program.load(emulator, location);
          }
        }
      });
    }
    if(count > 0) {
      recentGames.append(MenuSeparator());
      MenuItem clearHistory{&recentGames};
      clearHistory.setIcon(Icon::Edit::Clear);
      clearHistory.setText("Clear History");
      clearHistory.onActivate([&] {
        for(u32 index : range(9)) settings.recent.game[index] = {};
        loadEmulators();
      });
    } else {
      recentGames.setEnabled(false);
    }
  }
  loadMenu.append(MenuSeparator());

  //build emulator load list
  u32 enabled = 0;
  for(auto& emulator : emulators) {
    if(!emulator->configuration.visible) continue;
    enabled++;

    MenuItem item;
    item.setIcon(Icon::Place::Server);
    item.setText({emulator->name, " ..."});
    item.setVisible(emulator->configuration.visible);
    item.onActivate([=] {
      program.load(emulator);
    });
    if(settings.general.groupEmulators) {
      Menu menu;
      for(auto& action : loadMenu.actions()) {
        if(auto group = action.cast<Menu>()) {
          if(group.text() == emulator->manufacturer) {
            menu = group;
            break;
          }
        }
      }
      if(!menu) {
        menu.setIcon(Icon::Emblem::Folder);
        menu.setText(emulator->manufacturer);
        loadMenu.append(menu);
      }
      menu.append(item);
    } else {
      loadMenu.append(item);
    }
  }
  if(enabled == 0) {
    //if the user disables every system, give an indication for how to re-add systems to the load menu
    MenuItem item{&loadMenu};
    item.setIcon(Icon::Action::Add);
    item.setText("Add Systems ...");
    item.onActivate([&] {
      settingsWindow.show("Emulators");
    });
  }
  loadMenu.append(MenuSeparator());

  { MenuItem quit{&loadMenu};
    quit.setIcon(Icon::Action::Quit);
    quit.setText("Quit");
    quit.onActivate([&] {
      program.quit();
    });
  }
}

auto Presentation::loadEmulator() -> void {
  setTitle(emulator->root->game());

  systemMenu.setText(emulator->name);
  systemMenu.setVisible();

  //allow each emulator core to create any specialized menus necessary:
  //for instance, floppy disk and CD-ROM swapping support.
  emulator->load(systemMenu);
  if(systemMenu.actionCount() > 0) systemMenu.append(MenuSeparator());

  //todo: enable this once controller selection is supported
  #if 0
  u32 portsFound = 0;
  for(auto port : ares::Node::enumerate<ares::Node::Port>(emulator->root)) {
    if(!port->hotSwappable()) continue;
    if(port->type() != "Controller" && port->type() != "Expansion") continue;

    portsFound++;
    Menu portMenu{&systemMenu};
    if(port->type() == "Controller") portMenu.setIcon(Icon::Device::Joypad);
    if(port->type() == "Expansion" ) portMenu.setIcon(Icon::Device::Storage);
    portMenu.setText(port->name());

    Group peripheralGroup;
    { MenuRadioItem peripheralItem{&portMenu};
      peripheralItem.setText("Nothing");
      peripheralGroup.append(peripheralItem);
    }
    for(auto peripheral : port->supported()) {
      MenuRadioItem peripheralItem{&portMenu};
      peripheralItem.setText(peripheral);
      peripheralGroup.append(peripheralItem);
    }
  }
  if(portsFound > 0) systemMenu.append(MenuSeparator());
  #endif

  MenuItem reset{&systemMenu};
  reset.setText("Reset").setIcon(Icon::Action::Refresh).onActivate([&] {
    emulator->root->power();
    program.showMessage("System reset");
  });
  systemMenu.append(MenuSeparator());

  MenuItem unload{&systemMenu};
  unload.setText("Unload").setIcon(Icon::Media::Eject).onActivate([&] {
    program.unload();
    if(settings.video.adaptiveSizing) presentation.resizeWindow();
    presentation.showIcon(true);
  });

  toolsMenu.setVisible(true);
  pauseEmulation.setChecked(false);

  setFocused();
  viewport.setFocused();
}

auto Presentation::unloadEmulator(bool reloading) -> void {
  setTitle({ares::Name, " v", ares::Version});

  systemMenu.setVisible(false);
  systemMenu.reset();

  toolsMenu.setVisible(false);
}

auto Presentation::showIcon(bool visible) -> void {
  iconLayout.setVisible(visible);
  iconSpacer.setVisible(visible);
  iconCanvas.setVisible(visible);
  iconBottom.setVisible(visible);
  layout.resize();
}

auto Presentation::loadShaders() -> void {
  videoShaderMenu.reset();
  videoShaderMenu.setEnabled(ruby::video.hasShader());
  if(!ruby::video.hasShader()) return;

  Group shaders;

  MenuRadioItem none{&videoShaderMenu};
  none.setText("None").onActivate([&] {
    settings.video.shader = "None";
    ruby::video.setShader(settings.video.shader);
  });
  shaders.append(none);

  MenuRadioItem blur{&videoShaderMenu};
  blur.setText("Blur").onActivate([&] {
    settings.video.shader = "Blur";
    ruby::video.setShader(settings.video.shader);
  });
  shaders.append(blur);

  auto location = locate("Shaders/");

  if(ruby::video.driver() == "OpenGL 3.2") {
    for(auto shader : directory::folders(location, "*.shader")) {
      if(shaders.objectCount() == 2) videoShaderMenu.append(MenuSeparator());
      MenuRadioItem item{&videoShaderMenu};
      item.setText(string{shader}.trimRight(".shader/", 1L)).onActivate([=] {
        settings.video.shader = {location, shader};
        ruby::video.setShader(settings.video.shader);
      });
      shaders.append(item);
    }
  }

  if(settings.video.shader == "None") none.setChecked();
  if(settings.video.shader == "Blur") blur.setChecked();
  for(auto item : shaders.objects<MenuRadioItem>()) {
    if(settings.video.shader == string{location, item.text(), ".shader/"}) {
      item.setChecked();
    }
  }
}
