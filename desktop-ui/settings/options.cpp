auto OptionSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  rewind.setText("Rewind").setChecked(settings.general.rewind).onToggle([&] {
    settings.general.rewind = rewind.checked();
    program.rewindReset();
  }).doToggle();
  rewindLayout.setAlignment(1);
      rewindHint.setText("Allows you to reverse time via the rewind hotkey").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  runAhead.setText("Run-Ahead").setEnabled(co_serializable()).setChecked(settings.general.runAhead && co_serializable()).onToggle([&] {
    settings.general.runAhead = runAhead.checked() && co_serializable();
    program.runAheadUpdate();
  });
  runAheadLayout.setAlignment(1);
      runAheadHint.setText("Removes one frame of input lag, but doubles system requirements").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  autoSaveMemory.setText("Auto-Save Memory Periodically").setChecked(settings.general.autoSaveMemory).onToggle([&] {
    settings.general.autoSaveMemory = autoSaveMemory.checked();
  });
  autoSaveMemoryLayout.setAlignment(1);
      autoSaveMemoryHint.setText("Helps safeguard game saves from being lost").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  homebrewMode.setText("Homebrew Development Mode").setChecked(settings.general.homebrewMode).onToggle([&] {
    settings.general.homebrewMode = homebrewMode.checked();
  });
  homebrewModeLayout.setAlignment(1);
      homebrewModeHint.setText("Activate core-specific features to help homebrew developers").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);
}
