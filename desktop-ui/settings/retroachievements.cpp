#if defined(ARES_ENABLE_RCHEEVOS)
  #include <curl/curl.h>
#endif

#if defined(ARES_ENABLE_RCHEEVOS)
namespace {
auto avatarWrite(char* data, size_t size, size_t count, void* userdata) -> size_t {
  auto& output = *static_cast<std::vector<u8>*>(userdata);
  auto bytes = size * count;
  output.insert(output.end(), data, data + bytes);
  return bytes;
}

auto fetchAvatar(const string& avatarUrl) -> image {
  if(!avatarUrl) return {};

  string url = avatarUrl;
  if(url.beginsWith("/")) url = {"https://media.retroachievements.org", url};

  CURL* curl = curl_easy_init();
  if(!curl) return {};

  std::vector<u8> responseBody;
  curl_easy_setopt(curl, CURLOPT_URL, url.data());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, avatarWrite);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "ares");
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

  auto curlResult = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  if(curlResult != CURLE_OK || responseBody.empty()) return {};

  image avatar{responseBody};
  if(!avatar) return {};
  avatar.scale(48, 48);
  return avatar;
}
}
#endif

auto RetroAchievementsSettings::construct() -> void {
  setCollapsible();
  setVisible(false);

  accountLabel.setText("RetroAchievements").setFont(Font().setBold());

#if defined(ARES_ENABLE_RCHEEVOS)
  enabled.setText("Enable RetroAchievements").setChecked(settings.retroAchievements.enabled).onToggle([&] {
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
    refresh();
  });

  auto logout = [&] {
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
  if(!settings.retroAchievements.enabled || !settings.retroAchievements.username || !settings.retroAchievements.token) {
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
  loginButton.setEnabled(enabledSetting && !loggedIn);
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
      cachedAvatarImage = fetchAvatar(avatarUrl);
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

auto RetroAchievementsSettings::setVisible(bool visible) -> RetroAchievementsSettings& {
  if(visible) refresh();
  return VerticalLayout::setVisible(visible), *this;
}
