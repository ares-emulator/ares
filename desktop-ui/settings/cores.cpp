auto CoreSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  settingsHint.setText("Note: Settings changes require a game reload to take effect").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  nintendo64SettingsLabel.setText("Nintendo 64 Settings").setFont(Font().setBold());

  nintendo64ExpansionPakOption.setText("4MB Expansion Pak").setChecked(settings.nintendo64.expansionPak).onToggle([&] {
    settings.nintendo64.expansionPak = nintendo64ExpansionPakOption.checked();
  });
  nintendo64ExpansionPakLayout.setAlignment(1).setPadding(12_sx, 0);
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
  nintendo64ControllerPakBankLayout.setAlignment(1).setPadding(12_sx, 0);
    nintendo64ControllerPakBankLabel.setText("Controller Pak Size:");
    nintendo64ControllerPakBankHint.setText("Sets the size of a newly created Controller Pak's available memory").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

    renderQualityLayout.setPadding(12_sx, 0);

  disableVideoInterfaceProcessingOption.setText("Disable Video Interface Processing").setChecked(settings.nintendo64.disableVideoInterfaceProcessing).onToggle([&] {
    Program::Guard guard;
    settings.nintendo64.disableVideoInterfaceProcessing = disableVideoInterfaceProcessingOption.checked();
    if(emulator) emulator->setBoolean("Disable Video Interface Processing", settings.nintendo64.disableVideoInterfaceProcessing);
  });
  disableVideoInterfaceProcessingLayout.setAlignment(1).setPadding(12_sx, 0);
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
  weaveDeinterlacingLayout.setAlignment(1).setPadding(12_sx, 0);
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
  renderSupersamplingLayout.setAlignment(1).setPadding(12_sx, 0);
  renderSupersamplingHint.setText("Scales 2x and 4x resolutions back down to native.").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  #if !defined(VULKAN)
  //hide Vulkan-specific options if Vulkan is not available
  renderQualityLayout.setCollapsible(true).setVisible(false);
  renderSupersamplingLayout.setCollapsible(true).setVisible(false);
  disableVideoInterfaceProcessingLayout.setCollapsible(true).setVisible(false);
  weaveDeinterlacingLayout.setCollapsible(true).setVisible(false);
  #endif

  gameBoyAdvanceSettingsLabel.setText("Game Boy Advance Settings").setFont(Font().setBold());
  gameBoyPlayerOption.setText("Game Boy Player").setChecked(settings.gameBoyAdvance.player).onToggle([&] {
    settings.gameBoyAdvance.player = gameBoyPlayerOption.checked();
  });
  gameBoyPlayerLayout.setAlignment(1).setPadding(12_sx, 0);
    gameBoyPlayerHint.setText("Enable/Disable Game Boy Player rumble").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  superFamicomSettingsLabel.setText("Super Famicom Settings").setFont(Font().setBold());
  superFamicomDeepBlackBoostOption.setText("Deep Black Boost").setChecked(settings.superFamicom.deepBlackBoost).onToggle([&] {
    Program::Guard guard;
    settings.superFamicom.deepBlackBoost = superFamicomDeepBlackBoostOption.checked();
    if(emulator) emulator->setBoolean("Deep Black Boost", settings.superFamicom.deepBlackBoost);
  });
  superFamicomDeepBlackBoostLayout.setAlignment(1).setPadding(12_sx, 0);
  superFamicomDeepBlackBoostHint.setText("Applies a gamma ramp to crush black levels").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  megaDriveSettingsLabel.setText("Mega Drive Settings").setFont(Font().setBold());
  megaDriveTmssOption.setText("TMSS Boot Rom").setChecked(settings.megadrive.tmss).onToggle([&] {
    settings.megadrive.tmss = megaDriveTmssOption.checked();
  });
  megaDriveTmssLayout.setAlignment(1).setPadding(12_sx, 0);
    megaDriveTmssHint.setText("Enable/Disable the TMSS Boot Rom at system initialization").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
}
