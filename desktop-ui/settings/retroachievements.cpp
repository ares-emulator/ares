auto RetroAchievementsSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  accountLabel.setText("RetroAchievements").setFont(Font().setBold());

#if defined(ARES_ENABLE_RCHEEVOS)
  enabled.setText("Enable RetroAchievements").setChecked(settings.retroAchievements.enabled).onToggle([&] {
    settings.retroAchievements.enabled = enabled.checked();
    settings.save();
    program.showMessage(settings.retroAchievements.enabled ? "[RA] Enabled" : "[RA] Disabled");
  });

  usernameLabel.setText("Username:");
  usernameValue.setText(settings.retroAchievements.username).onChange([&] {
    settings.retroAchievements.username = usernameValue.text();
  });

  passwordLabel.setText("Password:");
  passwordValue.setText();

  statusLabel.setText(settings.retroAchievements.token ? "Logged in" : "Enter your RA password");

  loginButton.setText("Login").onActivate([&] {
    settings.retroAchievements.username = usernameValue.text();
    auto result = retroAchievements.login(settings.retroAchievements.username, passwordValue.text());
    passwordValue.setText();

    if(result.success) {
      settings.retroAchievements.enabled = true;
      settings.retroAchievements.username = result.username;
      settings.retroAchievements.token = result.token;
      enabled.setChecked(true);
      usernameValue.setText(result.username);
      settings.save();
    }

    statusLabel.setText(result.message);
    program.showMessage({"[RA] ", result.message});
  });

  clearButton.setText("Clear").onActivate([&] {
    settings.retroAchievements.enabled = false;
    settings.retroAchievements.username = "";
    settings.retroAchievements.token = "";
    enabled.setChecked(false);
    usernameValue.setText();
    passwordValue.setText();
    settings.save();
    statusLabel.setText("Logged out");
    program.showMessage("[RA] Login cleared");
  });
#else
  enabled.setText("Enable RetroAchievements").setEnabled(false);
  usernameLabel.setText("Username:");
  usernameValue.setText(settings.retroAchievements.username).setEnabled(false);
  passwordLabel.setText("Password:");
  passwordValue.setText().setEnabled(false);
  statusLabel.setText("Rebuild with ARES_ENABLE_RCHEEVOS to use RetroAchievements");
  loginButton.setText("Login").setEnabled(false);
  clearButton.setText("Clear").onActivate([&] {
    settings.retroAchievements.enabled = false;
    settings.retroAchievements.username = "";
    settings.retroAchievements.token = "";
    settings.save();
    usernameValue.setText();
    passwordValue.setText();
    statusLabel.setText("Logged out");
  });
#endif
}
