auto CoreSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  settingsHint.setText("Note: Settings changes require a game reload to take effect").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  gameBoyAdvanceTab.setText("Game Boy Advance");
  megaDriveTab.setText("Mega Drive");
  neoGeoTab.setText("Neo Geo");
  nintendo64Tab.setText("Nintendo 64");
  playstationTab.setText("PlayStation");
  superFamicomTab.setText("Super Famicom");
  for(auto panel : array<VerticalLayout[6]>{gameBoyAdvancePanel, megaDrivePanel, neoGeoPanel, nintendo64Panel, playstationPanel, superFamicomPanel}) {
    panel.setPadding(12_sx, 12_sy);
  }


  nintendo64ExpansionPakOption.setText("4MB Expansion Pak").setChecked(settings.nintendo64.expansionPak).onToggle([&] {
    settings.nintendo64.expansionPak = nintendo64ExpansionPakOption.checked();
  });
  nintendo64ExpansionPakLayout.setAlignment(1);
    nintendo64ExpansionPakHint.setText("Enable/Disable the 4MB Expansion Pak").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  for(auto& opt : array<string[4]>{"32KiB (Default)", "128KiB (Datel 1Meg)", "512KiB (Datel 4Meg)", "1984KiB (Maximum)"}) {
    ComboButtonItem item{&nintendo64ControllerPakBankOption};
    item.setText(opt);
    if (opt == settings.nintendo64.controllerPakBankString) {
      item.setSelected();

      if (opt == "32KiB (Default)") {
        settings.nintendo64.controllerPakBankCount = 1;
      } else if (opt == "128KiB (Datel 1Meg)") {
        settings.nintendo64.controllerPakBankCount = 4;
      } else if (opt == "512KiB (Datel 4Meg)") {
        settings.nintendo64.controllerPakBankCount = 16;
      } else if (opt == "1984KiB (Maximum)") {
        settings.nintendo64.controllerPakBankCount = 62;
      }
    }
  }
  nintendo64ControllerPakBankOption.onChange([&] {
    auto idx = nintendo64ControllerPakBankOption.selected();
    auto value = idx.text();
    if (value != settings.nintendo64.controllerPakBankString) {
      settings.nintendo64.controllerPakBankString = value;
      
      if (value == "32KiB (Default)") {
        settings.nintendo64.controllerPakBankCount = 1;
      } else if (value == "128KiB (Datel 1Meg)") {
        settings.nintendo64.controllerPakBankCount = 4;
      } else if (value == "512KiB (Datel 4Meg)") {
        settings.nintendo64.controllerPakBankCount = 16;
      } else if (value == "1984KiB (Maximum)") {
        settings.nintendo64.controllerPakBankCount = 62;
      }
    }
  });
  nintendo64ControllerPakBankLayout.setAlignment(0.5);
    nintendo64ControllerPakBankLabel.setText("Controller Pak Size:");
    nintendo64ControllerPakBankHint.setText("Sets the size of a newly created Controller Pak's available memory").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);


  disableVideoInterfaceProcessingOption.setText("Disable Video Interface Processing").setChecked(settings.nintendo64.disableVideoInterfaceProcessing).onToggle([&] {
    Program::Guard guard;
    settings.nintendo64.disableVideoInterfaceProcessing = disableVideoInterfaceProcessingOption.checked();
    if(emulator) emulator->setBoolean("Disable Video Interface Processing", settings.nintendo64.disableVideoInterfaceProcessing);
  });
  disableVideoInterfaceProcessingLayout.setAlignment(1);
  disableVideoInterfaceProcessingHint.setText("Disables Video Interface post processing to render image from VRAM directly").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  weaveDeinterlacingOption.setText("Weave Deinterlacing").setChecked(settings.nintendo64.weaveDeinterlacing).onToggle([&] {
    settings.nintendo64.weaveDeinterlacing = weaveDeinterlacingOption.checked();
    Program::Guard guard;
    if(emulator) emulator->setBoolean("(Experimental) Double the perceived vertical resolution; disabled when supersampling is used", settings.nintendo64.weaveDeinterlacing);
    if(weaveDeinterlacingOption.checked() == true) {
      renderSupersamplingOption.setChecked(false).setEnabled(false);
      settings.nintendo64.supersampling = false;
    } else {
      if(settings.nintendo64.quality != "SD") renderSupersamplingOption.setEnabled(true);
    }
  });
  weaveDeinterlacingLayout.setAlignment(1);
  weaveDeinterlacingHint.setText("Doubles the perceived vertical resolution; incompatible with supersampling").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  renderQuality1x.setText("1x Native").onActivate([&] {
    settings.nintendo64.quality = "SD";
    renderSupersamplingOption.setChecked(false).setEnabled(false);
    settings.nintendo64.supersampling = false;
    weaveDeinterlacingOption.setEnabled(true);
  });
  renderQuality2x.setText("2x Native").onActivate([&] {
    settings.nintendo64.quality = "HD";
    if(weaveDeinterlacingOption.checked() == false) renderSupersamplingOption.setChecked(settings.nintendo64.supersampling).setEnabled(true);
  });
  renderQuality4x.setText("4x Native").onActivate([&] {
    settings.nintendo64.quality = "UHD";
    if(weaveDeinterlacingOption.checked() == false) renderSupersamplingOption.setChecked(settings.nintendo64.supersampling).setEnabled(true);
  });
  if(settings.nintendo64.quality == "SD") renderQuality1x.setChecked();
  if(settings.nintendo64.quality == "HD") renderQuality2x.setChecked();
  if(settings.nintendo64.quality == "UHD") renderQuality4x.setChecked();
  renderSupersamplingOption.setText("Supersampling").setChecked(settings.nintendo64.supersampling && settings.nintendo64.quality != "SD").setEnabled(settings.nintendo64.quality != "SD").onToggle([&] {
    settings.nintendo64.supersampling = renderSupersamplingOption.checked();
    if(renderSupersamplingOption.checked() == true) {
      weaveDeinterlacingOption.setEnabled(false).setChecked(false);
      settings.nintendo64.weaveDeinterlacing = false;
    } else {
      weaveDeinterlacingOption.setEnabled(true);
    }
  });
  renderSupersamplingLayout.setAlignment(1);
  renderSupersamplingHint.setText("Scales 2x and 4x resolutions back down to native.").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  #if !defined(VULKAN)
  //hide Vulkan-specific options if Vulkan is not available
  renderQualityLayout.setCollapsible(true).setVisible(false);
  renderSupersamplingLayout.setCollapsible(true).setVisible(false);
  disableVideoInterfaceProcessingLayout.setCollapsible(true).setVisible(false);
  weaveDeinterlacingLayout.setCollapsible(true).setVisible(false);
  #endif

  playstationExpansionLabel.setText("Parallel Port:");
  playstationExpansionLayout.setAlignment(0.5);
  { ComboButtonItem item{&playstationExpansionOption};
    item.setText("None");
    item.setAttribute<string>("device", "None");
    if(settings.playstation.expansionDevice == "None") item.setSelected(); }
  for(auto& emulator : emulators) {
    if(emulator->name != "PlayStation") continue;
    for(auto& entry : emulator->firmware) {
      if(entry.type == "BIOS") continue;
      ComboButtonItem item{&playstationExpansionOption};
      item.setText({entry.type, " *"});
      item.setAttribute<string>("device", entry.type);
      if(entry.type == settings.playstation.expansionDevice) item.setSelected();
    }
  }
  playstationExpansionOption.onChange([&] {
    if(auto item = playstationExpansionOption.selected()) settings.playstation.expansionDevice = item.attribute<string>("device");
  });
  playstationFirmwareHint.setText("* Requires firmware in Firmware settings.").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
  playstationEEPROMHint.setText("EEPROM changes are stored in save data.\nChanging the assigned firmware resets saved EEPROM changes.").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
  playstationCompatibilityHint.setText("Warning: Some copy-protected games may not boot with a cheat device connected,\neven when its switch is off.").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  gameBoyPlayerOption.setText("Game Boy Player").setChecked(settings.gameBoyAdvance.player).onToggle([&] {
    settings.gameBoyAdvance.player = gameBoyPlayerOption.checked();
  });
  gameBoyPlayerLayout.setAlignment(1);
    gameBoyPlayerHint.setText("Enable/Disable Game Boy Player rumble").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  superFamicomDeepBlackBoostOption.setText("Deep Black Boost").setChecked(settings.superFamicom.deepBlackBoost).onToggle([&] {
    Program::Guard guard;
    settings.superFamicom.deepBlackBoost = superFamicomDeepBlackBoostOption.checked();
    if(emulator) emulator->setBoolean("Deep Black Boost", settings.superFamicom.deepBlackBoost);
  });
  superFamicomDeepBlackBoostLayout.setAlignment(1);
  superFamicomDeepBlackBoostHint.setText("Applies a gamma ramp to crush black levels").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  megaDriveTmssOption.setText("TMSS Boot Rom").setChecked(settings.megadrive.tmss).onToggle([&] {
    settings.megadrive.tmss = megaDriveTmssOption.checked();
  });
  megaDriveTmssLayout.setAlignment(1);
    megaDriveTmssHint.setText("Enable/Disable the TMSS Boot Rom at system initialization").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  neoGeoAesBiosLabel.setText("AES BIOS (aes.zip):");
  neoGeoMvsBiosLabel.setText("MVS BIOS (neogeo.zip):");
  neoGeoAesBiosLayout.setAlignment(0.5);
  neoGeoMvsBiosLayout.setAlignment(0.5);

  auto database = BML::unserialize(file::read(locate("Database/Arcade.bml")));
  auto populateBios = [&](string setName, ComboButton& selector, string& selection) {
    for(auto game : database.find("game")) {
      if(game["name"].string() != setName) continue;
      string defaultRom;
      bool selected = false;
      for(auto bios : game.find("bios")) {
        auto label = bios["name"].string();
        auto rom = bios["rom"].string();
        if(!label || !rom) continue;
        ComboButtonItem item{&selector};
        item.setText(label).setAttribute<string>("bios", rom);
        if(rom == selection) item.setSelected(), selected = true;
        if(bios["default"].boolean()) defaultRom = rom;
      }
      if(!selected) {
        for(auto item : selector.items()) {
          if(item.attribute<string>("bios") != defaultRom) continue;
          item.setSelected();
          selection = defaultRom;
          break;
        }
      }
      break;
    }
  };
  populateBios("aes", neoGeoAesBiosOption, settings.neoGeo.aesBios);
  neoGeoAesBiosOption.onChange([&] {
    if(auto item = neoGeoAesBiosOption.selected()) settings.neoGeo.aesBios = item.attribute<string>("bios");
  });

  populateBios("neogeo", neoGeoMvsBiosOption, settings.neoGeo.mvsBios);
  neoGeoMvsBiosOption.onChange([&] {
    if(auto item = neoGeoMvsBiosOption.selected()) settings.neoGeo.mvsBios = item.attribute<string>("bios");
  });
}
