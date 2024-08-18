#include "../desktop-ui.hpp"
namespace Instances { Instance<Presentation> presentation; }
Presentation& presentation = Instances::presentation();

#define ELLIPSIS "\u2026"

Presentation::Presentation() {
  loadMenu.setText("Load");

  systemMenu.setVisible(false);

  settingsMenu.setText("Settings");
  videoSizeMenu.setText("Size").setIcon(Icon::Emblem::Image);

  //generate size menu
  u32 multipliers = 5;
  for(u32 multiplier : range(1, multipliers + 1)) {
    MenuRadioItem item{&videoSizeMenu};
    item.setText({multiplier, "x"});
    item.onActivate([=] {
      settings.video.multiplier = multiplier;
      resizeWindow();
    });

    videoSizeGroup.append(item);
  }

  for(auto& item : videoSizeGroup.objects<MenuRadioItem>()) {
    if(item.text() == string{settings.video.multiplier, "x"}) item.setChecked();
  }

  videoSizeMenu.append(MenuSeparator());
  MenuItem centerWindow{&videoSizeMenu};
  centerWindow.setText("Center Window").setIcon(Icon::Place::Settings).onActivate([&] {
    setAlignment(Alignment::Center);
  });
  videoOutputMenu.setText("Output").setIcon(Icon::Emblem::Image);
  videoOutputPixelPerfect.setText("Pixel Perfect").onActivate([&] {
    settings.video.output = "Perfect";
  });
  videoOutputFixedScale.setText("Scale (Fixed)").onActivate([&] {
    settings.video.output = "Fixed";
  });
  videoOutputIntegerScale.setText("Scale (Integer)").onActivate([&] {
    settings.video.output = "Integer";
  });
  videoOutputScale.setText("Scale (Best Fit)").onActivate([&] {
    settings.video.output = "Scale";
  });
  videoOutputStretch.setText("Stretch").onActivate([&] {
    settings.video.output = "Stretch";
  });

  if(settings.video.output == "Perfect" ) videoOutputPixelPerfect.setChecked();
  if(settings.video.output == "Fixed"   ) videoOutputFixedScale.setChecked();
  if(settings.video.output == "Integer" ) videoOutputIntegerScale.setChecked();
  if(settings.video.output == "Scale"   ) videoOutputScale.setChecked();
  if(settings.video.output == "Stretch" ) videoOutputStretch.setChecked();

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
  videoSettingsAction.setText("Video" ELLIPSIS).setIcon(Icon::Device::Display).onActivate([&] {
    settingsWindow.show("Video");
  });
  audioSettingsAction.setText("Audio" ELLIPSIS).setIcon(Icon::Device::Speaker).onActivate([&] {
    settingsWindow.show("Audio");
  });
  inputSettingsAction.setText("Input" ELLIPSIS).setIcon(Icon::Device::Joypad).onActivate([&] {
    settingsWindow.show("Input");
  });
  hotkeySettingsAction.setText("Hotkeys" ELLIPSIS).setIcon(Icon::Device::Keyboard).onActivate([&] {
    settingsWindow.show("Hotkeys");
  });
  emulatorSettingsAction.setText("Emulators" ELLIPSIS).setIcon(Icon::Place::Server).onActivate([&] {
    settingsWindow.show("Emulators");
  });
  optionSettingsAction.setText("Options" ELLIPSIS).setIcon(Icon::Action::Settings).onActivate([&] {
    settingsWindow.show("Options");
  });
  firmwareSettingsAction.setText("Firmware" ELLIPSIS).setIcon(Icon::Emblem::Binary).onActivate([&] {
    settingsWindow.show("Firmware");
  });
  pathSettingsAction.setText("Paths" ELLIPSIS).setIcon(Icon::Emblem::Folder).onActivate([&] {
    settingsWindow.show("Paths");
  });
  driverSettingsAction.setText("Drivers" ELLIPSIS).setIcon(Icon::Place::Settings).onActivate([&] {
    settingsWindow.show("Drivers");
  });
  debugSettingsAction.setText("Debug" ELLIPSIS).setIcon(Icon::Device::Network).onActivate([&] {
    settingsWindow.show("Debug");
  });

  toolsMenu.setVisible(false).setText("Tools");
  saveStateMenu.setText("Save State").setIcon(Icon::Media::Record);
  for(u32 slot : range(9)) {
    MenuItem item{&saveStateMenu};
    item.setText({"Slot ", 1 + slot}).onActivate([=] {
      if(program.stateSave(1 + slot)) {
        undoSaveStateMenu.setEnabled(true);
      }
    });
  }
  loadStateMenu.setText("Load State").setIcon(Icon::Media::Rewind);
  for(u32 slot : range(9)) {
    MenuItem item{&loadStateMenu};
    item.setText({"Slot ", 1 + slot}).onActivate([=] {
      if(program.stateLoad(1 + slot)) {
        undoLoadStateMenu.setEnabled(true);
      }
    });
  }
  undoSaveStateMenu.setText("Undo Last Save State").setIcon(Icon::Edit::Undo).setEnabled(false);
  undoSaveStateMenu.onActivate([&] {
    program.undoStateSave();
    undoSaveStateMenu.setEnabled(false);
  });
  undoLoadStateMenu.setText("Undo Last Load State").setIcon(Icon::Edit::Undo).setEnabled(false);
  undoLoadStateMenu.onActivate([&] {
    program.undoStateLoad();
    undoLoadStateMenu.setEnabled(false);
  });
  captureScreenshot.setText("Capture Screenshot").setIcon(Icon::Emblem::Image).onActivate([&] {
    program.requestScreenshot = true;
  });
  pauseEmulation.setText("Pause Emulation").onToggle([&] {
    program.pause(!program.paused);
  });
  reloadGame.setText("Reload Game").setIcon(Icon::Action::Refresh).onActivate([&] {
    program.load(emulator, emulator->game->location);
  });
  frameAdvance.setText("Frame Advance").setIcon(Icon::Media::Play).onActivate([&] {
    if (!program.paused) program.pause(true);
    program.requestFrameAdvance = true;
  });
  manifestViewerAction.setText("Manifest").setIcon(Icon::Emblem::Binary).onActivate([&] {
    toolsWindow.show("Manifest");
  });
  cheatEditorAction.setText("Cheats").setIcon(Icon::Emblem::File).onActivate([&] {
    toolsWindow.show("Cheats");
  });
  #if !defined(PLATFORM_MACOS)
  // Cocoa hiro is missing the hex editor widget
  memoryEditorAction.setText("Memory").setIcon(Icon::Device::Storage).onActivate([&] {
    toolsWindow.show("Memory");
  });
  #endif
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
  aboutAction.setText("About" ELLIPSIS).setIcon(Icon::Prompt::Question).onActivate([&] {
    multiFactorImage logo(Resource::Ares::Logo1x, Resource::Ares::Logo2x);
    AboutDialog()
    .setName(ares::Name)
    .setLogo(logo)
    .setDescription({ares::Name, " — a simplified multi-system emulator"})
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
    
  Application::onOpenFile([&](auto filename) {
    if(auto emulator = program.identify(filename)) {
      program.load(emulator, filename);
    }
  });

  iconLayout.setCollapsible();

  iconSpacer.setCollapsible().setColor({0, 0, 0}).setDroppable().onDrop([&](auto filenames) {
    viewport.doDrop(filenames);
  });

  multiFactorImage icon(Resource::Ares::Icon1x, Resource::Ares::Icon2x);
  icon.alphaBlend(0x000000);
  iconCanvas.setCollapsible().setIcon(icon).setDroppable().onDrop([&](auto filenames) {
    viewport.doDrop(filenames);
  });

  iconPadding.setCollapsible().setColor({0, 0, 0}).setDroppable().onDrop([&](auto filenames) {
    viewport.doDrop(filenames);
  });

  iconBottom.setCollapsible().setColor({0, 0, 0}).setDroppable().onDrop([&](auto filenames) {
    viewport.doDrop(filenames);
  });

  spacerLeft .setBackgroundColor({32, 32, 32});
  statusLeft .setBackgroundColor({32, 32, 32}).setForegroundColor({255, 255, 255});
  statusDebug.setBackgroundColor({32, 32, 32}).setForegroundColor({255, 255, 255});
  statusRight.setBackgroundColor({32, 32, 32}).setForegroundColor({255, 255, 255});
  spacerRight.setBackgroundColor({32, 32, 32});

  statusLeft .setAlignment(0.0).setFont(Font().setBold());
  statusDebug.setAlignment(1.0).setFont(Font().setBold());
  statusRight.setAlignment(1.0).setFont(Font().setBold());

  onClose([&] {
    program.quit();
  });

  loadEmulators();

  resizeWindow();
  setTitle({ares::Name, " v", ares::Version});
  setAssociatedFile();
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
  if(fullScreen()) setFullScreen(false);
  if(maximized()) return;
  if(settings.video.output == "Fixed") return;

  u32 multiplier = settings.video.multiplier;
  u32 viewportWidth = 320 * multiplier;
  u32 viewportHeight = 240 * multiplier;

  if(emulator && program.screens) {
    auto& node = program.screens.first();
    u32 videoWidth = node->width() * node->scaleX();
    u32 videoHeight = node->height() * node->scaleY();
    if(settings.video.aspectCorrection) videoWidth = videoWidth * node->aspectX() / node->aspectY();
    if(node->rotation() == 90 || node->rotation() == 270) swap(videoWidth, videoHeight);

    viewportWidth = videoWidth * multiplier;
    viewportHeight = videoHeight * multiplier;
  }

  u32 statusHeight = showStatusBarSetting.checked() ? StatusHeight : 0;

  // Prevent the window frame from going out of bounds
  u32 monitorHeight = 1;
  u32 monitorWidth = 1;
  for(u32 monitor : range(Monitor::count())) {
    monitorHeight = max(monitorHeight, Monitor::workspace(monitor).height());
  }
  for(u32 monitor : range(Monitor::count())) {
    monitorWidth = max(monitorWidth, Monitor::workspace(monitor).width());
  }

  if(viewportWidth > monitorWidth || viewportHeight > monitorHeight) {
    // setMaximized causes odd window glitches and is not supported on macOS, avoid!
    // 100px buffer to account for possible taskbars
    setGeometry(Alignment::Center, {monitorWidth, monitorHeight - statusHeight - 100});
    return;
  }

  if(settings.video.autoCentering) {
    setGeometry(Alignment::Center, {viewportWidth, viewportHeight + statusHeight});
  } else {
    setSize({viewportWidth, viewportHeight + statusHeight});
  }

  setMinimumSize({160, 144 + statusHeight});
}

auto Presentation::loadEmulators() -> void {
  loadMenu.reset();

  //clean up the recent games history first
  vector<string> recentGames;
  for(u32 index : range(9)) {
    auto entry = settings.recent.game[index];
    auto system = entry.split(";", 1L)(0);
    auto location = entry.split(";", 1L)(1);
    if(location.length()) {  //remove missing games
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
      item.setIconForFile(location);
      item.setText({Location::base(location).trimRight("/"), " (", system, ")"});
      item.onActivate([=] {
        if(!inode::exists(location)) {
          MessageDialog()
            .setTitle("Error")
            .setText({location, " does not exist"})
            .setAlignment(presentation)
            .error();

          //remove the entry from the recent games list
          settings.recent.game[index] = {};
          loadEmulators();
          return;
        }
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
      #if !defined(PLATFORM_MACOS)
      clearHistory.setText("Clear History");
      #else
      clearHistory.setText("Clear Menu");
      #endif
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

  //first pass; make sure "Arcade" is start of list
  for(auto& emulator : emulators) {
    if(!emulator->configuration.visible) continue;
    if(emulator->group() == "Arcade") {
      Menu menu;
      menu.setIcon(Icon::Emblem::Folder);
      menu.setText(emulator->group());
      loadMenu.append(menu);
      break;
    }
  }

  for(auto& emulator : emulators) {
    if (!emulator->configuration.visible) continue;
    enabled++;
    MenuItem item;
    item.setIcon(Icon::Place::Server);
    item.setText({emulator->name, ELLIPSIS});
    item.setVisible(emulator->configuration.visible);
    item.onActivate([=] {
      program.load(emulator);
    });

    Menu menu;
    for(auto& action : loadMenu.actions()) {
      if(auto group = action.cast<Menu>()) {
        if(group.text() == emulator->group()) {
          menu = group;
          break;
        }
      }
    }
    if(!menu) {
      menu.setIcon(Icon::Emblem::Folder);
      menu.setText(emulator->group());
      loadMenu.append(menu);
    }
    menu.append(item);
  }
  if(enabled == 0) {
    //if the user disables every system, give an indication for how to re-add systems to the load menu
    MenuItem item{&loadMenu};
    item.setIcon(Icon::Action::Add);
    item.setText("Add Systems" ELLIPSIS);
    item.onActivate([&] {
      settingsWindow.show("Emulators");
    });
  }

  #if !defined(PLATFORM_MACOS)
  loadMenu.append(MenuSeparator());

  { MenuItem quit{&loadMenu};
    quit.setIcon(Icon::Action::Quit);
    quit.setText("Quit");
    quit.onActivate([&] {
      program.quit();
    });
  }
  #endif
}

auto Presentation::loadEmulator() -> void {
  setTitle(emulator->root->game());
  setAssociatedFile(emulator->game->location);
  systemMenu.setText(emulator->name);
  systemMenu.setVisible();

  refreshSystemMenu();

  toolsMenu.setVisible(true);
  pauseEmulation.setChecked(false);

  setFocused();
  viewport.setFocused();
}

auto Presentation::refreshSystemMenu() -> void {
  systemMenu.reset();

  //allow each emulator core to create any specialized menus necessary:
  //for instance, floppy disk and CD-ROM swapping support.
  emulator->load(systemMenu);
  if(systemMenu.actionCount() > 0) systemMenu.append(MenuSeparator());

  u32 portsFound = 0;
  for(auto port : ares::Node::enumerate<ares::Node::Port>(emulator->root)) {
    //do not add unsupported ports to the port menu
    if(emulator->portBlacklist.find(port->name())) continue;

    if(!port->hotSwappable()) continue;
    if(port->type() != "Controller" && port->type() != "Expansion") continue;

    portsFound++;
    Menu portMenu{&systemMenu};
    if(port->type() == "Controller") portMenu.setIcon(Icon::Device::Joypad);
    if(port->type() == "Expansion" ) portMenu.setIcon(Icon::Device::Storage);
    portMenu.setText(port->name());

    Group peripheralGroup;
    { MenuRadioItem peripheralItem{&portMenu};
      peripheralItem.setAttribute<ares::Node::Port>("port", port);
      peripheralItem.setText("Nothing");
      peripheralItem.onActivate([=] {
        auto port = peripheralItem.attribute<ares::Node::Port>("port");
        port->disconnect();
        refreshSystemMenu();
      });
      peripheralGroup.append(peripheralItem);
    }
    for(auto peripheral : port->supported()) {
      //do not add unsupported peripherals to the peripheral port menu
      if(emulator->inputBlacklist.find(peripheral)) continue;

      MenuRadioItem peripheralItem{&portMenu};
      peripheralItem.setAttribute<ares::Node::Port>("port", port);
      peripheralItem.setText(peripheral);
      peripheralItem.onActivate([=] {
        auto port = peripheralItem.attribute<ares::Node::Port>("port");
        port->disconnect();
        port->allocate(peripheralItem.text());
        port->connect();
        refreshSystemMenu();
      });
      peripheralGroup.append(peripheralItem);
    }

    //check the peripheral item menu option that is currently connected to said port
    if(auto connected = port->connected()) {
      for(auto peripheralItem : peripheralGroup.objects<MenuRadioItem>()) {
        if(peripheralItem.text() == connected->name()) peripheralItem.setChecked();
      }
    }
  }

  if(portsFound > 0) systemMenu.append(MenuSeparator());

  MenuItem reset{&systemMenu};
  reset.setText("Reset").setIcon(Icon::Action::Refresh).onActivate([&] {
    emulator->root->power(true);
    program.showMessage("System reset");
  });
  systemMenu.append(MenuSeparator());

  MenuItem unload{&systemMenu};
  unload.setText("Unload").setIcon(Icon::Media::Eject).onActivate([&] {
  program.unload();
  if(settings.video.adaptiveSizing) presentation.resizeWindow();
    presentation.showIcon(true);
  });
}

auto Presentation::unloadEmulator(bool reloading) -> void {
  setTitle({ares::Name, " v", ares::Version});
  setAssociatedFile();
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

  MenuCheckItem none{&videoShaderMenu};
  none.setText("None").onToggle([&] {
    settings.video.shader = "None";
    ruby::video.setShader(settings.video.shader);
    loadShaders();
  });
  shaders.append(none);

  string location = locate("Shaders/");

  if(shaderDirectories.size() == 0) {
    function<void(string)> findShaderDirectories = [&](string path) {
      for(auto &entry: directory::folders(path)) findShaderDirectories({path, entry});
      auto files = directory::files(path, "*.slangp");
      if(files.size() > 0) shaderDirectories.append((string({path}).trimLeft(location, 1L)));
    };
    findShaderDirectories(location);

    // Sort by name and depth such that child folders appear after their parents
    shaderDirectories.sort([](const string &lhs, const string &rhs) {
      auto lhsParts = lhs.split("/");
      auto rhsParts = rhs.split("/");
      for(u32 i : range(min(lhsParts.size(), rhsParts.size()))) {
        if(lhsParts[i] != rhsParts[i]) return lhsParts[i] < rhsParts[i];
      }
      return lhsParts.size() < rhsParts.size();
    });
  }

  if(ruby::video.hasShader()) {
    for(auto &directory : shaderDirectories) {
      auto parts = directory.split("/");
      Menu parent = videoShaderMenu;

      if(directory != "") {
        for (auto &part: parts) {
          if(part == "") continue;
          Menu child;
          bool found = false;
          for(auto &action: parent.actions()) {
            if(auto menu = action.cast<Menu>()) {
              if(menu.text() == part) {
                child = menu;
                found = true;
                break;
              }
            }
          }

          if(found) {
            parent = child;
          } else {
            Menu newMenu{&parent};
            newMenu.setText(part);
            parent = newMenu;
          }
        }
      }

      auto files = directory::files({location, directory}, "*.slangp");
      for(auto &file: files) {
        MenuCheckItem item{&parent};
        item.setAttribute("file", {directory, file});
        item.setText(string{file}.trimRight(".slangp", 1L)).onToggle([=] {
          settings.video.shader = {directory, file};
          ruby::video.setShader({location, settings.video.shader});
          loadShaders();
        });
        shaders.append(item);
      }
    }
  }

  if(program.startShader) {
    string existingShader = settings.video.shader;

    if(!program.startShader.imatch("None")) {
      settings.video.shader = {location, program.startShader, ".slangp"};
    } else {
      settings.video.shader = program.startShader;
    }

    if(inode::exists(settings.video.shader)) {
      ruby::video.setShader({location, settings.video.shader});
      loadShaders();
    } else if(settings.video.shader.imatch("None")) {
      ruby::video.setShader("None");
      loadShaders();
    } else {
      hiro::MessageDialog()
          .setTitle("Warning")
          .setAlignment(hiro::Alignment::Center)
          .setText({ "Requested shader not found: ", settings.video.shader , "\nUsing existing defined shader: ", existingShader })
          .warning();
      settings.video.shader = existingShader;
    }
  }

  if(settings.video.shader.imatch("None")) {none.setChecked(); settings.video.shader = "None";}
  for(auto item : shaders.objects<MenuCheckItem>()) {
    if(settings.video.shader.imatch(item.attribute("file"))) {
      item.setChecked();
      settings.video.shader = item.attribute("file");
      ruby::video.setShader({location, settings.video.shader});
    }
  }
}
