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

auto PreferenceSettings::refresh() -> void {
  auto value = max(1u, min(Settings::Prefs::maxRecentGames, settings.prefs.recentGamesLimit));
  settings.prefs.recentGamesLimit = value;
  recentGamesValue.setText(integer(value));
}
