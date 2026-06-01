#if defined(ARES_ENABLE_RCHEEVOS)
  #include <curl/curl.h>
  #include <rc_api_user.h>

namespace {
struct RetroAchievementsLoginResult {
  bool success = false;
  string username;
  string token;
  string message;
};

auto retroAchievementsCurlWrite(char* data, size_t size, size_t count, void* userdata) -> size_t {
  auto& output = *static_cast<std::string*>(userdata);
  output.append(data, size * count);
  return size * count;
}

auto retroAchievementsLogin(const string& username, const string& password) -> RetroAchievementsLoginResult {
  RetroAchievementsLoginResult result;

  if(!username || !password) {
    result.message = "Enter username and password";
    return result;
  }

  rc_api_request_t request = {};
  rc_api_login_request_t loginRequest = {};
  loginRequest.username = username.data();
  loginRequest.password = password.data();

  if(auto rc = rc_api_init_login_request(&request, &loginRequest); rc != RC_OK) {
    result.message = {"Login request failed: ", rc_error_str(rc)};
    return result;
  }

  CURL* curl = curl_easy_init();
  if(!curl) {
    rc_api_destroy_request(&request);
    result.message = "Unable to initialize network request";
    return result;
  }

  std::string responseBody;
  curl_easy_setopt(curl, CURLOPT_URL, request.url);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.post_data);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, retroAchievementsCurlWrite);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "ares");
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
  if(request.content_type) {
    string contentType = {"Content-Type: ", request.content_type};
    auto* headers = curl_slist_append(nullptr, contentType.data());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    auto curlResult = curl_easy_perform(curl);
    long statusCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
    curl_slist_free_all(headers);

    if(curlResult != CURLE_OK) {
      result.message = {"Login failed: ", curl_easy_strerror(curlResult)};
      curl_easy_cleanup(curl);
      rc_api_destroy_request(&request);
      return result;
    }

    rc_api_server_response_t serverResponse = {};
    serverResponse.body = responseBody.c_str();
    serverResponse.body_length = responseBody.size();
    serverResponse.http_status_code = (int)statusCode;

    rc_api_login_response_t loginResponse = {};
    if(auto rc = rc_api_process_login_server_response(&loginResponse, &serverResponse); rc == RC_OK && loginResponse.response.succeeded) {
      result.success = true;
      result.username = loginResponse.username;
      result.token = loginResponse.api_token;
      result.message = {"Logged in as ", result.username};
    } else {
      result.message = loginResponse.response.error_message ? loginResponse.response.error_message : rc_error_str(rc);
    }
    rc_api_destroy_login_response(&loginResponse);
  }

  curl_easy_cleanup(curl);
  rc_api_destroy_request(&request);
  return result;
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
    auto result = retroAchievementsLogin(settings.retroAchievements.username, passwordValue.text());
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
