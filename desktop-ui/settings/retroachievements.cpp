auto RetroAchievementsSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  accountLabel.setText("RetroAchievements").setFont(Font().setBold());

#if defined(ARES_ENABLE_RCHEEVOS)
  enabled.setText("Enable RetroAchievements").setChecked(settings.retroAchievements.enabled).onToggle([&] {
    Program::Guard guard;
    settings.retroAchievements.enabled = enabled.checked();
    settings.save();
    if(settings.retroAchievements.enabled) retroAchievements.initialize();
    program.showMessage(settings.retroAchievements.enabled ? "[RA] Enabled" : "[RA] Disabled");
    refresh();
  });

  usernameLabel.setText("Username:");
  usernameLayout.setCollapsible();
  usernameValue.setText(settings.retroAchievements.username).onChange([&] {
    settings.retroAchievements.username = usernameValue.text();
  });

  passwordLabel.setText("Password:");
  passwordLayout.setCollapsible();
  passwordValue.setText();

  statusLabel.setText();
  actionLayout.setCollapsible();
  profileLayout.setCollapsible();
  profileName.setFont(Font().setBold());
  profilePoints.setForegroundColor(SystemColor::Sublabel);

  loginButton.setText("Login").onActivate([&] {
    Program::Guard guard;
    settings.retroAchievements.username = usernameValue.text();
    bool started = retroAchievements.login(settings.retroAchievements.username, passwordValue.text());
    passwordValue.setText();
    statusLabel.setText(started ? "Signing in..." : "Enter a username and password");
    loginButton.setEnabled(!started);
  });

  auto logout = [&] {
    Program::Guard guard;
    retroAchievements.logout();
    settings.retroAchievements.username = "";
    settings.retroAchievements.token = "";
    usernameValue.setText();
    passwordValue.setText();
    settings.save();
    statusLabel.setText("Logged out");
    program.showMessage("[RA] Login cleared");
    refresh();
  };
  clearButton.setText("Logout").onActivate(logout);
  profileLogoutButton.setText("Logout").onActivate(logout);
  refresh();
#else
  enabled.setText("Enable RetroAchievements").setEnabled(false);
  usernameLabel.setText("Username:");
  usernameValue.setText(settings.retroAchievements.username).setEnabled(false);
  passwordLabel.setText("Password:");
  passwordValue.setText().setEnabled(false);
  statusLabel.setText("Rebuild with ARES_ENABLE_RCHEEVOS to use RetroAchievements");
  loginButton.setText("Login").setEnabled(false);
  profileLogoutButton.setText("Logout").setEnabled(false);
  clearButton.setText("Clear").onActivate([&] {
    settings.retroAchievements.enabled = false;
    settings.retroAchievements.username = "";
    settings.retroAchievements.token = "";
    settings.save();
    usernameValue.setText();
    passwordValue.setText();
    statusLabel.setText("Logged out");
  });
  profileLayout.setVisible(false);
#endif
}

auto RetroAchievementsSettings::refresh() -> void {
#if defined(ARES_ENABLE_RCHEEVOS)
  if(!retroAchievements.loginPending() && (!settings.retroAchievements.enabled || !settings.retroAchievements.username || !settings.retroAchievements.token)) {
    retroAchievements.logout();
  }

  auto enabledSetting = settings.retroAchievements.enabled;
  auto loggedIn = enabledSetting && retroAchievements.hasUser() && settings.retroAchievements.username && settings.retroAchievements.token;

  enabled.setChecked(settings.retroAchievements.enabled).setEnabled(true);
  usernameLayout.setVisible(enabledSetting && !loggedIn);
  passwordLayout.setVisible(enabledSetting && !loggedIn);
  actionLayout.setVisible(enabledSetting && !loggedIn);
  profileLayout.setVisible(enabledSetting && loggedIn);
  usernameValue.setText(loggedIn ? retroAchievements.username() : settings.retroAchievements.username).setEnabled(enabledSetting && !loggedIn);
  passwordValue.setEnabled(enabledSetting && !loggedIn);
  loginButton.setEnabled(enabledSetting && !loggedIn && !retroAchievements.loginPending());
  clearButton.setEnabled(false);
  profileLogoutButton.setEnabled(enabledSetting && loggedIn);

  if(loggedIn) {
    auto score = retroAchievements.userScore();
    auto softcore = retroAchievements.userScoreSoftcore();
    auto hardcore = score >= softcore ? score - softcore : 0;
    profileName.setText(retroAchievements.displayName());
    profilePoints.setText({score, " points (SC: ", softcore, ", HC: ", hardcore, ")"});
    statusLabel.setText("Logged in");

    auto avatarUrl = retroAchievements.avatarUrl();
    if(avatarUrl != cachedAvatarUrl) {
      cachedAvatarUrl = avatarUrl;
      cachedAvatarImage = {};
      if(avatarUrl) {
        string url = avatarUrl;
        if(url.beginsWith("/")) url = {"https://media.retroachievements.org", url};
        retroAchievements.requestAvatar(url);
      }
    }
    profileAvatar.setIcon(cachedAvatarImage ? (multiFactorImage)cachedAvatarImage : (multiFactorImage)Icon::Action::Bookmark);
  } else if(!enabledSetting) {
    statusLabel.setText();
    passwordValue.setText();
    cachedAvatarUrl = {};
    cachedAvatarImage = {};
    profileAvatar.setIcon();
  } else {
    if(!statusLabel.text()) statusLabel.setText("Enter your RA password");
    passwordValue.setText();
    cachedAvatarUrl = {};
    cachedAvatarImage = {};
    profileAvatar.setIcon();
  }
  settingsWindow.panelContainer.resize();
#endif
}

auto RetroAchievementsSettings::acceptAvatar(const string& url, const std::vector<u8>& bytes) -> void {
#if defined(ARES_ENABLE_RCHEEVOS)
  string expectedUrl = cachedAvatarUrl;
  if(expectedUrl.beginsWith("/")) expectedUrl = {"https://media.retroachievements.org", expectedUrl};
  if(url != expectedUrl || bytes.empty()) return;
  cachedAvatarImage = image{bytes};
  if(!cachedAvatarImage) return;
  cachedAvatarImage.scale(48, 48);
  profileAvatar.setIcon((multiFactorImage)cachedAvatarImage);
#else
  (void)url;
  (void)bytes;
#endif
}

auto RetroAchievementsSettings::setVisible(bool visible) -> RetroAchievementsSettings& {
  if(visible) refresh();
  return VerticalLayout::setVisible(visible), *this;
}
