auto OptionSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  commonSettingsLabel.setText("Emulator Options").setFont(Font().setBold());

  rewind.setText("Rewind").setChecked(settings.general.rewind).onToggle([&] {
    settings.general.rewind = rewind.checked();
    program.rewindReset();
  }).doToggle();
  rewindLayout.setAlignment(1).setPadding(12_sx, 0);
      rewindHint.setText("Allows you to reverse time via the rewind hotkey").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  runAhead.setText("Run-Ahead").setEnabled(co_serializable()).setChecked(settings.general.runAhead && co_serializable()).onToggle([&] {
    settings.general.runAhead = runAhead.checked() && co_serializable();
    program.runAheadUpdate();
  });
  runAheadLayout.setAlignment(1).setPadding(12_sx, 0);
      runAheadHint.setText("Removes one frame of input lag, but doubles system requirements").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  autoSaveMemory.setText("Auto-Save Memory Periodically").setChecked(settings.general.autoSaveMemory).onToggle([&] {
    settings.general.autoSaveMemory = autoSaveMemory.checked();
  });
  autoSaveMemoryLayout.setAlignment(1).setPadding(12_sx, 0);
      autoSaveMemoryHint.setText("Helps safeguard game saves from being lost").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  homebrewMode.setText("Homebrew Development Mode").setChecked(settings.general.homebrewMode).onToggle([&] {
    settings.general.homebrewMode = homebrewMode.checked();
  });
  homebrewModeLayout.setAlignment(1).setPadding(12_sx, 0);
      homebrewModeHint.setText("Activate core-specific features to help homebrew developers").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  forceInterpreter.setText("Force Interpreter").setChecked(settings.general.forceInterpreter).onToggle([&] {
    settings.general.forceInterpreter = forceInterpreter.checked();
  });
  forceInterpreterLayout.setAlignment(1).setPadding(12_sx, 0);
      forceInterpreterHint.setText("(Slow) Enable interpreter for cores that default to a recompiler").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  nintendo64SettingsLabel.setText("Nintendo 64 Settings").setFont(Font().setBold());

  nintendo64ExpansionPakOption.setText("4MB Expansion Pak").setChecked(settings.nintendo64.expansionPak).onToggle([&] {
    settings.nintendo64.expansionPak = nintendo64ExpansionPakOption.checked();
  });
  nintendo64ExpansionPakLayout.setAlignment(1).setPadding(12_sx, 0);
      nintendo64ExpansionPakHint.setText("Enable/Disable the 4MB Expansion Pak").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  for (auto& opt : array<string[4]>{"32KiB (Default)", "128KiB (Datel 1Meg)", "512KiB (Datel 4Meg)", "1984KiB (Maximum)"}) {
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

  gameBoyAdvanceSettingsLabel.setText("Game Boy Advance Settings").setFont(Font().setBold());

  gameBoyPlayerOption.setText("Game Boy Player").setChecked(settings.gameBoyAdvance.player).onToggle([&] {
    settings.gameBoyAdvance.player = gameBoyPlayerOption.checked();
  });
  gameBoyPlayerLayout.setAlignment(1).setPadding(12_sx, 0);
    gameBoyPlayerHint.setText("Enable/Disable Game Boy Player rumble").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  megaDriveSettingsLabel.setText("Mega Drive Settings").setFont(Font().setBold());

  megaDriveTmssOption.setText("TMSS Boot Rom").setChecked(settings.megadrive.tmss).onToggle([&] {
    settings.megadrive.tmss = megaDriveTmssOption.checked();
  });
  megaDriveTmssLayout.setAlignment(1).setPadding(12_sx, 0);
    megaDriveTmssHint.setText("Enable/Disable the TMSS Boot Rom at system initialization").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

}
