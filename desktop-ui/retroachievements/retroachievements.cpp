#include "../desktop-ui.hpp"

#if defined(ARES_ENABLE_RCHEEVOS)
  #include "platform/adapter.hpp"
  #include <curl/curl.h>
  #include <rc_api_user.h>
  #include <rc_hash.h>
#endif

RetroAchievements retroAchievements;

#if defined(ARES_ENABLE_RCHEEVOS)
namespace {
auto generateHash(const Emulator& emu) -> string {
  auto* adapter = RA::Platform::selectAdapter(emu);
  if(!adapter) return {};

  std::vector<u8> rom;
  if(!adapter->romData(emu, rom) || rom.empty()) return {};

  char hash[33] = {};
  if(!rc_hash_generate_from_buffer(hash, adapter->consoleId(emu), rom.data(), rom.size())) return {};
  return hash;
}

auto loginMessage(const RetroAchievementsLoginResult& result) -> string {
  auto name = result.displayName ? result.displayName : result.username;
  u32 hardcoreScore = result.score >= result.scoreSoftcore ? result.score - result.scoreSoftcore : 0;
  return {
    name, " connected (", result.score, " pts. SC: ", result.scoreSoftcore,
    " pts, HC: ", hardcoreScore, " pts)"
  };
}

auto curlWrite(char* data, size_t size, size_t count, void* userdata) -> size_t {
  auto& output = *static_cast<std::string*>(userdata);
  output.append(data, size * count);
  return size * count;
}

auto loginRequest(const string& username, const string& password, const string& token) -> RetroAchievementsLoginResult {
  RetroAchievementsLoginResult result;

  if(!username || (!password && !token)) {
    result.message = token ? "Saved RetroAchievements login is incomplete" : "Enter username and password";
    return result;
  }

  rc_api_request_t request = {};
  rc_api_login_request_t loginRequest = {};
  loginRequest.username = username.data();
  if(password) loginRequest.password = password.data();
  if(token) loginRequest.api_token = token.data();

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
  curl_slist* headers = nullptr;

  curl_easy_setopt(curl, CURLOPT_URL, request.url);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.post_data);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "ares");
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

  if(request.content_type) {
    string contentType = {"Content-Type: ", request.content_type};
    headers = curl_slist_append(headers, contentType.data());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  }

  auto curlResult = curl_easy_perform(curl);
  long statusCode = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);

  if(headers) curl_slist_free_all(headers);

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
    result.displayName = loginResponse.display_name;
    result.avatarUrl = loginResponse.avatar_url;
    result.score = loginResponse.score;
    result.scoreSoftcore = loginResponse.score_softcore;
    result.message = loginMessage(result);
  } else {
    result.message = loginResponse.response.error_message ? loginResponse.response.error_message : rc_error_str(rc);
  }

  rc_api_destroy_login_response(&loginResponse);
  curl_easy_cleanup(curl);
  rc_api_destroy_request(&request);

  return result;
}
}
#endif

auto RetroAchievements::initialize() -> void {
#if defined(ARES_ENABLE_RCHEEVOS)
  if(!settings.retroAchievements.enabled) return;
  if(!settings.retroAchievements.username || !settings.retroAchievements.token) return;

  auto result = loginRequest(settings.retroAchievements.username, "", settings.retroAchievements.token);
  if(result.success) {
    setUser(result);
    settings.retroAchievements.username = result.username;
    settings.retroAchievements.token = result.token;
    settings.save();
  }
  program.showMessage({"[RA] ", result.message});
#endif
}

auto RetroAchievements::login(const string& username, const string& password) -> RetroAchievementsLoginResult {
#if defined(ARES_ENABLE_RCHEEVOS)
  auto result = loginRequest(username, password, "");
  if(result.success) setUser(result);
  return result;
#else
  RetroAchievementsLoginResult result;
  result.message = "Rebuild with ARES_ENABLE_RCHEEVOS to use RetroAchievements";
  return result;
#endif
}

auto RetroAchievements::logout() -> void {
  clearUser();
}

auto RetroAchievements::hasUser() const -> bool {
  return _authenticated;
}

auto RetroAchievements::username() const -> string {
  return _username;
}

auto RetroAchievements::displayName() const -> string {
  return _displayName ? _displayName : _username;
}

auto RetroAchievements::userScore() const -> u32 {
  return _score;
}

auto RetroAchievements::userScoreSoftcore() const -> u32 {
  return _scoreSoftcore;
}

auto RetroAchievements::avatarUrl() const -> string {
  return _avatarUrl;
}

auto RetroAchievements::readMemory(u32 address, u8* buffer, u32 size) const -> u32 {
#if defined(ARES_ENABLE_RCHEEVOS)
  if(!emulator) return 0;
  if(!settings.retroAchievements.enabled) return 0;
  if(auto* adapter = RA::Platform::selectAdapter(*emulator)) {
    return adapter->readMemory(*emulator, address, buffer, size);
  }
#else
  (void)address;
  (void)buffer;
  (void)size;
#endif
  return 0;
}

auto RetroAchievements::gameLoaded() -> void {
  _gameHash = {};

#if defined(ARES_ENABLE_RCHEEVOS)
  if(!emulator) return;
  if(!settings.retroAchievements.enabled) return;

  auto hash = generateHash(*emulator);
  if(!hash) {
    if(RA::Platform::selectAdapter(*emulator)) {
      program.showMessage("[RA] Could not hash loaded game");
    }
    return;
  }

  _gameHash = hash;
  program.showMessage({"[RA] Game hash: ", _gameHash});
#endif
}

auto RetroAchievements::gameUnloaded() -> void {
  _gameHash = {};
}

auto RetroAchievements::gameHash() const -> string {
  return _gameHash;
}

auto RetroAchievements::setUser(const RetroAchievementsLoginResult& result) -> void {
  _authenticated = true;
  _username = result.username;
  _displayName = result.displayName;
  _avatarUrl = result.avatarUrl;
  _score = result.score;
  _scoreSoftcore = result.scoreSoftcore;
}

auto RetroAchievements::clearUser() -> void {
  _authenticated = false;
  _username = {};
  _displayName = {};
  _avatarUrl = {};
  _score = 0;
  _scoreSoftcore = 0;
}
