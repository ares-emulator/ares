#include "../desktop-ui.hpp"

#if defined(ARES_ENABLE_RCHEEVOS)
  #include "platform/adapter.hpp"
  #include <curl/curl.h>
  #include <rc_api_user.h>
  #include <rc_client.h>
  #include <rc_hash.h>
#endif

RetroAchievements retroAchievements;

#if defined(ARES_ENABLE_RCHEEVOS)
namespace {
struct ClientCallbackData {
  RetroAchievements* self = nullptr;
};

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

auto clientReadMemory(u32 address, u8* buffer, u32 size, rc_client_t* client) -> u32 {
  if(!client) return 0;
  auto* self = static_cast<RetroAchievements*>(rc_client_get_userdata(client));
  if(!self) return 0;
  return self->readMemory(address, buffer, size);
}

auto clientServerCall(const rc_api_request_t* request, rc_client_server_callback_t callback, void* callbackData, rc_client_t*) -> void {
  rc_api_server_response_t response = {};
  response.http_status_code = RC_API_SERVER_RESPONSE_CLIENT_ERROR;

  if(!request || !callback || !request->url) {
    if(callback) callback(&response, callbackData);
    return;
  }

  CURL* curl = curl_easy_init();
  if(!curl) {
    callback(&response, callbackData);
    return;
  }

  std::string responseBody;
  curl_slist* headers = nullptr;

  curl_easy_setopt(curl, CURLOPT_URL, request->url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWrite);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "ares/retroachievements-minimal");
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

  if(request->post_data) {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->post_data);
  }

  if(request->content_type) {
    string contentType = {"Content-Type: ", request->content_type};
    headers = curl_slist_append(headers, contentType.data());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  }

  auto curlResult = curl_easy_perform(curl);
  long statusCode = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
  if(headers) curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if(curlResult == CURLE_OK) {
    response.body = responseBody.c_str();
    response.body_length = responseBody.size();
    response.http_status_code = (int)statusCode;
  }

  callback(&response, callbackData);
}

auto clientEvent(const rc_client_event_t* event, rc_client_t*) -> void {
  if(!event) return;
  switch(event->type) {
  case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
    if(event->achievement && event->achievement->title) {
      program.showMessage({"[RA] Achievement unlocked: ", event->achievement->title});
    }
    break;
  case RC_CLIENT_EVENT_GAME_COMPLETED:
    program.showMessage("[RA] Game completed");
    break;
  case RC_CLIENT_EVENT_SERVER_ERROR:
    if(event->server_error && event->server_error->error_message) {
      program.showMessage({"[RA] Server error: ", event->server_error->error_message});
    }
    break;
  default:
    break;
  }
}

auto clientLoginCallback(int result, const char* errorMessage, rc_client_t* client, void* userdata) -> void {
  std::unique_ptr<ClientCallbackData> data{static_cast<ClientCallbackData*>(userdata)};
  if(!data || !data->self || !client) return;
  data->self->handleClientLoginResult(result, errorMessage);
}

auto clientLoadGameCallback(int result, const char* errorMessage, rc_client_t* client, void* userdata) -> void {
  std::unique_ptr<ClientCallbackData> data{static_cast<ClientCallbackData*>(userdata)};
  if(!data || !data->self || !client) return;
  data->self->handleClientGameLoadResult(result, errorMessage);
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

RetroAchievements::~RetroAchievements() {
#if defined(ARES_ENABLE_RCHEEVOS)
  unloadClientGame();
  if(_client) {
    rc_client_destroy(_client);
    _client = nullptr;
  }
#endif
}

auto RetroAchievements::ensureClient() -> bool {
#if defined(ARES_ENABLE_RCHEEVOS)
  if(_client) return true;
  _client = rc_client_create(clientReadMemory, clientServerCall);
  if(!_client) return false;
  rc_client_set_userdata(_client, this);
  rc_client_set_event_handler(_client, clientEvent);
  rc_client_set_allow_background_memory_reads(_client, 0);
  rc_client_set_hardcore_enabled(_client, 0);
  return true;
#else
  return false;
#endif
}

auto RetroAchievements::syncClientLogin() -> void {
#if defined(ARES_ENABLE_RCHEEVOS)
  if(!_authenticated) return;
  if(!_username || !_token) return;
  if(!ensureClient()) return;

  if(auto user = rc_client_get_user_info(_client)) {
    if(user->username && _username == user->username) return;
  }

  rc_client_begin_login_with_token(_client, _username, _token, clientLoginCallback, new ClientCallbackData{this});
#endif
}

auto RetroAchievements::loadGame() -> void {
#if defined(ARES_ENABLE_RCHEEVOS)
  _gameReady = false;
  _gameLoadPending = false;
  if(!settings.retroAchievements.enabled) return;
  if(!_authenticated) return;
  if(!emulator) return;
  if(!ensureClient()) return;

  auto* adapter = RA::Platform::selectAdapter(*emulator);
  if(!adapter) return;

  std::vector<u8> rom;
  if(!adapter->romData(*emulator, rom) || rom.empty()) {
    program.showMessage("[RA] Could not read ROM data for game lookup");
    return;
  }

  auto path = emulator->game ? emulator->game->location : string{};
  _gameLoadPending = true;
  rc_client_begin_identify_and_load_game(
    _client,
    adapter->consoleId(*emulator),
    path ? (const char*)path : nullptr,
    rom.data(),
    rom.size(),
    clientLoadGameCallback,
    new ClientCallbackData{this}
  );
#endif
}

auto RetroAchievements::unloadClientGame() -> void {
#if defined(ARES_ENABLE_RCHEEVOS)
  _gameReady = false;
  _gameLoadPending = false;
  if(_client) rc_client_unload_game(_client);
#endif
}

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
    syncClientLogin();
    if(_gameHash) loadGame();
  }
  program.showMessage({"[RA] ", result.message});
#endif
}

auto RetroAchievements::login(const string& username, const string& password) -> RetroAchievementsLoginResult {
#if defined(ARES_ENABLE_RCHEEVOS)
  auto result = loginRequest(username, password, "");
  if(result.success) {
    setUser(result);
    syncClientLogin();
    if(_gameHash) loadGame();
  }
  return result;
#else
  RetroAchievementsLoginResult result;
  result.message = "Rebuild with ARES_ENABLE_RCHEEVOS to use RetroAchievements";
  return result;
#endif
}

auto RetroAchievements::logout() -> void {
#if defined(ARES_ENABLE_RCHEEVOS)
  unloadClientGame();
  if(_client) rc_client_logout(_client);
#endif
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

auto RetroAchievements::handleClientLoginResult(int result, const char* errorMessage) -> void {
#if defined(ARES_ENABLE_RCHEEVOS)
  if(result != RC_OK) {
    _gameReady = false;
    program.showMessage({"[RA] Client login failed: ", errorMessage ? errorMessage : rc_error_str(result)});
    return;
  }

  if(auto user = _client ? rc_client_get_user_info(_client) : nullptr) {
    program.showMessage({"[RA] Client session ready for ", user->display_name ? user->display_name : user->username});
  }
#else
  (void)result;
  (void)errorMessage;
#endif
}

auto RetroAchievements::handleClientGameLoadResult(int result, const char* errorMessage) -> void {
#if defined(ARES_ENABLE_RCHEEVOS)
  _gameLoadPending = false;
  _gameReady = result == RC_OK && _client && rc_client_is_game_loaded(_client);

  if(result != RC_OK) {
    _gameReady = false;
    if(result == RC_NO_GAME_LOADED) {
      program.showMessage("[RA] Game not recognized by RetroAchievements");
    } else {
      program.showMessage({"[RA] Game load failed: ", errorMessage ? errorMessage : rc_error_str(result)});
    }
    return;
  }

  auto title = string{"Game loaded"};
  if(auto game = rc_client_get_game_info(_client)) {
    if(game->title) title = game->title;
  }

  u32 subsetCount = 0;
  if(auto subsets = rc_client_create_subset_list(_client)) {
    subsetCount = subsets->num_subsets;
    rc_client_destroy_subset_list(subsets);
  }

  rc_client_user_game_summary_t summary = {};
  rc_client_get_user_game_summary(_client, &summary);
  program.showMessage({
    "[RA] Game loaded: ", title,
    " (", summary.num_unlocked_achievements, "/",
    summary.num_core_achievements + summary.num_unofficial_achievements,
    " achievements, ", subsetCount, " sets)"
  });
#else
  (void)result;
  (void)errorMessage;
#endif
}

auto RetroAchievements::gameLoaded() -> void {
  _gameHash = {};

#if defined(ARES_ENABLE_RCHEEVOS)
  if(!emulator) return;
  if(!settings.retroAchievements.enabled) return;
  unloadClientGame();

  auto hash = generateHash(*emulator);
  if(!hash) {
    if(RA::Platform::selectAdapter(*emulator)) {
      program.showMessage("[RA] Could not hash loaded game");
    }
    return;
  }

  _gameHash = hash;
  program.showMessage({"[RA] Game hash: ", _gameHash});
  loadGame();
#endif
}

auto RetroAchievements::gameUnloaded() -> void {
  _gameHash = {};
  unloadClientGame();
}

auto RetroAchievements::gameHash() const -> string {
  return _gameHash;
}

auto RetroAchievements::setUser(const RetroAchievementsLoginResult& result) -> void {
  _authenticated = true;
  _username = result.username;
  _token = result.token;
  _displayName = result.displayName;
  _avatarUrl = result.avatarUrl;
  _score = result.score;
  _scoreSoftcore = result.scoreSoftcore;
}

auto RetroAchievements::clearUser() -> void {
  _authenticated = false;
  _username = {};
  _token = {};
  _displayName = {};
  _avatarUrl = {};
  _score = 0;
  _scoreSoftcore = 0;
}
