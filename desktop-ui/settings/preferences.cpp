auto PreferenceSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  historyLabel.setText("History").setFont(Font().setBold());

  recentGamesLayout.setAlignment(1).setPadding(12_sx, 0);
    recentGamesLabel.setText("Recent Games");
    recentGamesValue.setEditable(true);
    recentGamesValue.onChange([&] {
      if(!recentGamesValue.text().strip()) return;
      this->applyRecentGamesLimit(false);
    }).onActivate([&] {
      this->applyRecentGamesLimit(true);
    });
  recentGamesHintLayout.setAlignment(1).setPadding(12_sx, 0);
    recentGamesHint.setText("Maximum stored and shown entries (1-30)").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  startupLabel.setText("Startup").setFont(Font().setBold());

  restoreWindowStateLayout.setAlignment(1).setPadding(12_sx, 0);
    restoreWindowStateOption.setText("Restore window size and position").setChecked(settings.prefs.restoreWindowState).onToggle([&] {
      settings.prefs.restoreWindowState = restoreWindowStateOption.checked();
    });
    restoreWindowStateHint.setText("Reuse the previous window location and size on startup").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  implicitKioskLayout.setAlignment(1).setPadding(12_sx, 0);
    implicitKioskOption.setText("Imply --kiosk when opening a game").setChecked(settings.prefs.implicitKiosk).onToggle([&] {
      if(!implicitKioskOption.checked()) {
        settings.prefs.implicitKiosk = false;
        return;
      }

      auto response = MessageDialog()
        .setTitle("Warning")
        .setAlignment(settingsWindow)
        .setText("Launching ares with a game path will imply --kiosk.\n"
                 "Kiosk mode has no GUI, so you may have to manually edit ares' configfile to undo this.\n\n"
                 "Enable this option?")
        .warning({"Enable", "Cancel"});

      if(response == "Enable") {
        settings.prefs.implicitKiosk = true;
      } else {
        implicitKioskOption.setChecked(false);
        settings.prefs.implicitKiosk = false;
      }

      refresh();
    });
    implicitKioskHint.setText("Launch with --kiosk automatically when a game path is passed to ares").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  startGameFullScreenLayout.setAlignment(1).setPadding(12_sx, 0);
    startGameFullScreenOption.setText("Always start games in fullscreen").setChecked(settings.prefs.startGameFullScreen).onToggle([&] {
      settings.prefs.startGameFullScreen = startGameFullScreenOption.checked();
      if(settings.prefs.startGameFullScreen) {
        settings.prefs.startGamePseudoFullScreen = false;
      }
      refresh();
    });
    startGameFullScreenHint.setText("Apply native fullscreen after launching or reopening a game").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  startGamePseudoFullScreenLayout.setAlignment(1).setPadding(12_sx, 0);
    startGamePseudoFullScreenOption.setText("Always start games in pseudo-fullscreen").setChecked(settings.prefs.startGamePseudoFullScreen).onToggle([&] {
      settings.prefs.startGamePseudoFullScreen = startGamePseudoFullScreenOption.checked();
      if(settings.prefs.startGamePseudoFullScreen) {
        settings.prefs.startGameFullScreen = false;
      }
      refresh();
    });
    startGamePseudoFullScreenHint.setText("Apply pseudo-fullscreen after launching or reopening a game").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  resumeLastGameLayout.setAlignment(1).setPadding(12_sx, 0);
    resumeLastGameOption.setText("Reopen last loaded game on startup").setChecked(settings.prefs.resumeLastGame).onToggle([&] {
      settings.prefs.resumeLastGame = resumeLastGameOption.checked();
      updateResumeLastGamePausedState();
    });
    resumeLastGameHint.setText("Automatically reload the last opened game when ares starts").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  resumeLastGamePausedLayout.setAlignment(1).setPadding(12_sx, 0);
    resumeLastGamePausedOption.setText("Start reopened game paused").setChecked(settings.prefs.resumeLastGamePaused).onToggle([&] {
      settings.prefs.resumeLastGamePaused = resumeLastGamePausedOption.checked();
    });
    resumeLastGamePausedHint.setText("Only applies when reopening the last loaded game").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  menuLabel.setText("Menus").setFont(Font().setBold());

  showDisabledEmulatorsLayout.setAlignment(1).setPadding(12_sx, 0);
    showDisabledEmulatorsOption.setText("Show disabled emulators in Load menu").setChecked(settings.prefs.showDisabledEmulators).onToggle([&] {
      settings.prefs.showDisabledEmulators = showDisabledEmulatorsOption.checked();
      presentation.loadEmulators();
    });
    showDisabledEmulatorsHint.setText("Keep hidden systems visible in the Load menu").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  doubleClickFullScreenLayout.setAlignment(1).setPadding(12_sx, 0);
    doubleClickFullScreenOption.setText("Double-click viewport to toggle fullscreen").setChecked(settings.prefs.doubleClickFullScreen).onToggle([&] {
      settings.prefs.doubleClickFullScreen = doubleClickFullScreenOption.checked();
    });
    doubleClickFullScreenHint.setText("Toggle fullscreen by double-clicking on the game image").setFont(Font().setSize(7.0)).setForegroundColor(SystemColor::Sublabel);

  refresh();
}

auto PreferenceSettings::readRecentGamesLimit() const -> maybe<u32> {
  auto text = recentGamesValue.text().strip();
  if(!text) {
    return {};
  }

  for(auto character : text) {
    if(character < '0' || character > '9') return {};
  }

  auto value = text.natural();
  value = max(1u, min(Settings::Prefs::maxRecentGames, value));
  return value;
}

auto PreferenceSettings::applyRecentGamesLimit(bool normalizeField) -> void {
  auto value = readRecentGamesLimit();
  if(!value) {
    if(normalizeField) refresh();
    return;
  }

  settings.prefs.recentGamesLimit = *value;
  if(normalizeField) {
    recentGamesValue.setText(integer(*value));
  }
}

auto PreferenceSettings::updateResumeLastGamePausedState() -> void {
  auto enabled = settings.prefs.resumeLastGame;
  if(!enabled) {
    settings.prefs.resumeLastGamePaused = false;
  }
  resumeLastGamePausedOption.setEnabled(enabled);
  resumeLastGamePausedHint.setEnabled(enabled);
  resumeLastGamePausedOption.setChecked(enabled && settings.prefs.resumeLastGamePaused);
}

auto PreferenceSettings::refresh() -> void {
  auto value = max(1u, min(Settings::Prefs::maxRecentGames, settings.prefs.recentGamesLimit));
  settings.prefs.recentGamesLimit = value;
  recentGamesValue.setText(integer(value));
  restoreWindowStateOption.setChecked(settings.prefs.restoreWindowState);
  implicitKioskOption.setChecked(settings.prefs.implicitKiosk);
  startGameFullScreenOption.setChecked(settings.prefs.startGameFullScreen);
  startGamePseudoFullScreenOption.setChecked(settings.prefs.startGamePseudoFullScreen);
  implicitKioskHint.setEnabled(true);
  resumeLastGameOption.setChecked(settings.prefs.resumeLastGame);
  showDisabledEmulatorsOption.setChecked(settings.prefs.showDisabledEmulators);
  doubleClickFullScreenOption.setChecked(settings.prefs.doubleClickFullScreen);
  updateResumeLastGamePausedState();
}
