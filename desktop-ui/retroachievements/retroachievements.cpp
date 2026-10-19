#include "../desktop-ui.hpp"

#include "platform/adapter.hpp"
#include <curl/curl.h>
#include <rc_api_user.h>
#include <rc_client.h>
#include <rc_hash.h>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace {
struct ClientCallbackData {
  RetroAchievements* self = nullptr;
  u64 generation = 0;
};

struct AchievementCounts {
  u32 unlocked = 0;
  u32 total = 0;
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

auto achievementUnlocked(const rc_client_achievement_t* achievement) -> bool {
  if(!achievement) return false;
  return achievement->unlocked & (RC_CLIENT_ACHIEVEMENT_UNLOCKED_SOFTCORE | RC_CLIENT_ACHIEVEMENT_UNLOCKED_HARDCORE);
}

auto achievementIsWarning(const rc_client_achievement_t* achievement) -> bool {
  if(!achievement) return false;
  auto title = achievement->title ? string{achievement->title} : string{};
  return achievement->id == 101000001 || (achievement->points == 0 && title.beginsWith("Warning:"));
}

auto visibleAchievementCounts(rc_client_t* client) -> AchievementCounts {
  AchievementCounts counts;
  if(!client) return counts;

  auto* list = rc_client_create_achievement_list(
    client,
    RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE_AND_UNOFFICIAL,
    RC_CLIENT_ACHIEVEMENT_LIST_GROUPING_LOCK_STATE
  );
  if(!list) return counts;

  for(u32 bucketIndex : range(list->num_buckets)) {
    auto& bucket = list->buckets[bucketIndex];
    for(u32 achievementIndex : range(bucket.num_achievements)) {
      auto* achievement = bucket.achievements[achievementIndex];
      if(!achievement || achievementIsWarning(achievement)) continue;
      counts.total++;
      if(achievementUnlocked(achievement)) counts.unlocked++;
    }
  }

  rc_client_destroy_achievement_list(list);
  return counts;
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
  case RC_CLIENT_EVENT_RESET:
    retroAchievements.resetGame();
    break;
  default:
    break;
  }
}

auto clientLoginCallback(int result, const char* errorMessage, rc_client_t* client, void* userdata) -> void {
  std::unique_ptr<ClientCallbackData> data{static_cast<ClientCallbackData*>(userdata)};
  if(!data || !data->self || !client) return;
  data->self->handleClientLoginResult(result, errorMessage, data->generation);
}

auto clientLoadGameCallback(int result, const char* errorMessage, rc_client_t* client, void* userdata) -> void {
  std::unique_ptr<ClientCallbackData> data{static_cast<ClientCallbackData*>(userdata)};
  if(!data || !data->self || !client) return;
  data->self->handleClientGameLoadResult(result, errorMessage, data->generation);
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
  string userAgent = {"ares/", ares::Version};
  curl_easy_setopt(curl, CURLOPT_USERAGENT, (const char*)userAgent);
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
    result.errorCode = rc;
    result.message = loginResponse.response.error_message ? loginResponse.response.error_message : rc_error_str(rc);
  }

  rc_api_destroy_login_response(&loginResponse);
  curl_easy_cleanup(curl);
  rc_api_destroy_request(&request);

  return result;
}

struct HttpWorker {
  struct Task {
    enum class Kind { Client, Login, Avatar } kind;
    std::string url;
    std::string postData;
    std::string contentType;
    std::string username;
    std::string password;
    std::string token;
    bool post = false;
    u64 generation = 0;
    rc_client_server_callback_t callback = nullptr;
    void* callbackData = nullptr;
  };

  struct ClientResponse {
    std::vector<char> body;
    int status = RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR;
    rc_client_server_callback_t callback = nullptr;
    void* callbackData = nullptr;
  };
  struct LoginResponse {
    u64 generation = 0;
    RetroAchievementsLoginResult result;
  };
  struct AvatarResponse {
    string url;
    std::vector<u8> bytes;
  };

  std::mutex mutex;
  std::condition_variable cv;
  std::deque<Task> tasks;
  std::deque<ClientResponse> clientResponses;
  std::deque<LoginResponse> loginResponses;
  std::deque<AvatarResponse> avatarResponses;
  std::thread thread;
  bool started = false;
  bool stopping = false;

  auto start() -> bool {
    if(started) return true;
    if(curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) return false;
    stopping = false;
    started = true;
    thread = std::thread([this] { run(); });
    return true;
  }

  auto enqueue(Task task) -> void {
    std::lock_guard lock{mutex};
    if(stopping) {
      if(task.kind == Task::Kind::Client) {
        clientResponses.push_back({{}, RC_API_SERVER_RESPONSE_CLIENT_ERROR, task.callback, task.callbackData});
      }
      return;
    }
    tasks.push_back(std::move(task));
    cv.notify_one();
  }

  auto takeClientResponses() -> std::deque<ClientResponse> {
    std::lock_guard lock{mutex};
    std::deque<ClientResponse> out;
    out.swap(clientResponses);
    return out;
  }

  auto takeLoginResponses() -> std::deque<LoginResponse> {
    std::lock_guard lock{mutex};
    std::deque<LoginResponse> out;
    out.swap(loginResponses);
    return out;
  }

  auto takeAvatarResponses() -> std::deque<AvatarResponse> {
    std::lock_guard lock{mutex};
    std::deque<AvatarResponse> out;
    out.swap(avatarResponses);
    return out;
  }

  auto pumpClient() -> void {
    for(auto& item : takeClientResponses()) {
      if(!item.callback) continue;
      rc_api_server_response_t response{};
      response.body = item.body.empty() ? nullptr : item.body.data();
      response.body_length = item.body.size();
      response.http_status_code = item.status;
      item.callback(&response, item.callbackData);
    }
  }

  auto stop() -> void {
    if(!started) return;
    {
      std::lock_guard lock{mutex};
      stopping = true;
      for(auto& task : tasks) {
        if(task.kind == Task::Kind::Client) {
          clientResponses.push_back({{}, RC_API_SERVER_RESPONSE_CLIENT_ERROR, task.callback, task.callbackData});
        }
      }
      tasks.clear();
    }
    cv.notify_one();
    if(thread.joinable()) thread.join();
    while(true) {
      {
        std::lock_guard lock{mutex};
        if(clientResponses.empty()) break;
      }
      pumpClient();
    }
    curl_global_cleanup();
    started = false;
  }

  auto run() -> void {
    while(true) {
      Task task;
      {
        std::unique_lock lock{mutex};
        cv.wait(lock, [&] { return stopping || !tasks.empty(); });
        if(stopping && tasks.empty()) return;
        task = std::move(tasks.front());
        tasks.pop_front();
      }

      if(task.kind == Task::Kind::Login) {
        auto result = loginRequest(task.username.c_str(), task.password.c_str(), task.token.c_str());
        std::lock_guard lock{mutex};
        loginResponses.push_back({task.generation, std::move(result)});
        continue;
      }

      CURL* curl = curl_easy_init();
      std::vector<char> body;
      int status = RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR;
      if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, task.url.c_str());
        string userAgent = {"ares/", ares::Version};
        curl_easy_setopt(curl, CURLOPT_USERAGENT, (const char*)userAgent);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](char* data, size_t size, size_t count, void* userdata) -> size_t {
          auto* out = (std::vector<char>*)userdata;
          auto bytes = size * count;
          out->insert(out->end(), data, data + bytes);
          return bytes;
        });
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
        curl_slist* headers = nullptr;
        if(task.kind == Task::Kind::Client && !task.contentType.empty()) {
          std::string contentType = "Content-Type: " + task.contentType;
          headers = curl_slist_append(headers, contentType.c_str());
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
        if(task.post) {
          curl_easy_setopt(curl, CURLOPT_POST, 1L);
          curl_easy_setopt(curl, CURLOPT_POSTFIELDS, task.postData.c_str());
          curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)task.postData.size());
        }
        if(curl_easy_perform(curl) == CURLE_OK) {
          long code = 0;
          curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
          status = code ? (int)code : RC_API_SERVER_RESPONSE_RETRYABLE_CLIENT_ERROR;
        } else {
          body.clear();
        }
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
      }

      std::lock_guard lock{mutex};
      if(task.kind == Task::Kind::Client) {
        clientResponses.push_back({std::move(body), status, task.callback, task.callbackData});
      } else {
        std::vector<u8> bytes;
        if(status >= 200 && status < 300) bytes.assign(body.begin(), body.end());
        avatarResponses.push_back({task.url.c_str(), std::move(bytes)});
      }
    }
  }
};

HttpWorker httpWorker;

struct StartupLoginDialog {
  auto construct() -> void {
    if(constructed) return;
    layout.setPadding(10_sx, 10_sy).setSpacing(8_sy);
    message.setText("Sign in to RetroAchievements to enable achievements for this session.")
      .setForegroundColor(SystemColor::Sublabel);

    usernameLayout.setAlignment(1).setSpacing(8_sx);
    usernameLabel.setText("Username:");
    usernameValue.onActivate([&] { passwordValue.setFocused(); });
    passwordLayout.setAlignment(1).setSpacing(8_sx);
    passwordLabel.setText("Password:");
    passwordValue.setMasked(true).onActivate([&] { submit(); });

    status.setForegroundColor(SystemColor::Sublabel).setVisible(false);
    buttons.setAlignment(1).setSpacing(8_sx);
    spacer.setCollapsible();
    loginButton.setText("Login").onActivate([&] { submit(); });
    cancelButton.setText("Cancel").onActivate([&] { close(true); });

    window.onClose([&] { close(true); });
    window.setDismissable();
    window.setTitle("RetroAchievements Login");
    constructed = true;
  }

  auto request(const string& text) -> void {
    if(dismissed) return;
    pending = true;
    statusText = text;
  }

  auto close(bool userDismissed = false) -> void {
    pending = false;
    if(userDismissed) dismissed = true;
    if(constructed && window.visible()) {
      passwordValue.setText();
      window.setVisible(false);
    }
  }

  auto setPending(bool loginPending) -> void {
    if(controlsPending == loginPending) return;
    controlsPending = loginPending;
    usernameValue.setEnabled(!loginPending);
    passwordValue.setEnabled(!loginPending);
    loginButton.setEnabled(!loginPending);
    cancelButton.setEnabled(!loginPending);
  }

  auto setStatus(const string& text, bool error = false) -> void {
    status.setText(text).setVisible((bool)text);
    status.setForegroundColor(error ? Color{224, 92, 92} : SystemColor::Sublabel);
  }

  auto show() -> void {
    construct();
    pending = false;
    usernameValue.setText(settings.retroAchievements.username);
    passwordValue.setText();
    setStatus(statusText);
    setPending(false);
    window.setSize({420_sx, layout.minimumSize().height()});
    window.setAlignment(presentation);
    window.setVisible();
    if(usernameValue.text()) passwordValue.setFocused();
    else usernameValue.setFocused();
  }

  auto submit() -> void {
    auto username = usernameValue.text().strip();
    auto password = passwordValue.text();
    if(!username) return setStatus("Enter a username before logging in.", true);
    if(!password) return setStatus("Enter a password to log in.", true);
    if(!retroAchievements.login(username, password)) return setStatus("Could not start login.", true);
    passwordValue.setText();
    setStatus("Signing in...");
    setPending(true);
  }

  auto pump() -> void {
    if(program.kiosk || !settings.retroAchievements.enabled) {
      close();
      return;
    }
    if(pending && !dismissed && !retroAchievements.loginPending() && !retroAchievements.hasUser()) show();
    if(constructed && window.visible()) setPending(retroAchievements.loginPending());
  }

  bool constructed = false;
  bool pending = false;
  bool dismissed = false;
  bool controlsPending = false;
  string statusText;
  Window window;
  VerticalLayout layout{&window};
  Label message{&layout, Size{~0, 0}};
  HorizontalLayout usernameLayout{&layout, Size{~0, 0}};
  Label usernameLabel{&usernameLayout, Size{80_sx, 0}};
  LineEdit usernameValue{&usernameLayout, Size{~0, 0}};
  HorizontalLayout passwordLayout{&layout, Size{~0, 0}};
  Label passwordLabel{&passwordLayout, Size{80_sx, 0}};
  LineEdit passwordValue{&passwordLayout, Size{~0, 0}};
  Label status{&layout, Size{~0, 0}};
  HorizontalLayout buttons{&layout, Size{~0, 0}};
  Canvas spacer{&buttons, Size{~0, 0}};
  Button loginButton{&buttons, Size{80_sx, 0}};
  Button cancelButton{&buttons, Size{80_sx, 0}};
};

StartupLoginDialog startupLoginDialog;

auto clientServerCall(const rc_api_request_t* request, rc_client_server_callback_t callback, void* callbackData, rc_client_t*) -> void {
  if(!callback) return;
  if(!request || !request->url) {
    rc_api_server_response_t response{};
    response.http_status_code = RC_API_SERVER_RESPONSE_CLIENT_ERROR;
    callback(&response, callbackData);
    return;
  }

  HttpWorker::Task task;
  task.kind = HttpWorker::Task::Kind::Client;
  task.url = request->url;
  if(request->post_data) task.postData = request->post_data;
  if(request->content_type) task.contentType = request->content_type;
  task.post = request->post_data != nullptr;
  task.callback = callback;
  task.callbackData = callbackData;
  httpWorker.enqueue(std::move(task));
}
}

RetroAchievements retroAchievements;

RetroAchievements::~RetroAchievements() {
  unloadClientGame();
  httpWorker.stop();
  if(_client) {
    rc_client_destroy(_client);
    _client = nullptr;
  }
}

auto RetroAchievements::ensureClient() -> bool {
  if(_client) return true;
  if(!httpWorker.start()) return false;
  _client = rc_client_create(clientReadMemory, clientServerCall);
  if(!_client) return false;
  rc_client_set_userdata(_client, this);
  rc_client_set_event_handler(_client, clientEvent);
  rc_client_set_allow_background_memory_reads(_client, 0);
  rc_client_set_hardcore_enabled(_client, settings.retroAchievements.hardcore ? 1 : 0);
  return true;
}

auto RetroAchievements::syncClientLogin() -> void {
  if(!_authenticated) return;
  if(!_username || !_token) return;
  if(!ensureClient()) return;

  if(auto user = rc_client_get_user_info(_client)) {
    if(user->username && _username == user->username) return;
  }

  rc_client_begin_login_with_token(_client, _username, _token, clientLoginCallback, new ClientCallbackData{this, _loginGeneration});
}

auto RetroAchievements::loadGame() -> void {
  _gameReady = false;
  _gameLoadPending = false;
  if(!settings.retroAchievements.enabled) return;
  if(!_authenticated) return;
  if(!emulator) return;
  if(!ensureClient()) return;
  rc_client_set_hardcore_enabled(_client, hardcore() ? 1 : 0);

  auto* adapter = RA::Platform::selectAdapter(*emulator);
  if(!adapter) return;

  _pendingRomData.clear();
  if(!adapter->romData(*emulator, _pendingRomData) || _pendingRomData.empty()) {
    program.showMessage("[RA] Could not read ROM data for game lookup");
    return;
  }

  auto path = emulator->game ? emulator->game->location : string{};
  _gameLoadPending = true;
  rc_client_begin_identify_and_load_game(
    _client,
    adapter->consoleId(*emulator),
    path ? (const char*)path : nullptr,
    _pendingRomData.data(),
    _pendingRomData.size(),
    clientLoadGameCallback,
    new ClientCallbackData{this, _gameGeneration}
  );
}

auto RetroAchievements::unloadClientGame() -> void {
  ++_gameGeneration;
  _gameReady = false;
  _gameLoadPending = false;
  if(_client) rc_client_unload_game(_client);
  _pendingRomData.clear();
}

auto RetroAchievements::initialize() -> void {
  Program::Guard guard;
  if(!settings.retroAchievements.enabled) return;
  startupLoginDialog.dismissed = false;
  if(!settings.retroAchievements.username || !settings.retroAchievements.token) {
    if(program.kiosk) program.showMessage("[RA] Sign in through the main GUI to enable achievements");
    else startupLoginDialog.request("RetroAchievements login required.");
    return;
  }
  if(!beginSavedTokenLogin(true)) {
    if(!program.kiosk) startupLoginDialog.request("RetroAchievements network initialization failed.");
  }
}

auto RetroAchievements::beginSavedTokenLogin(bool startup) -> bool {
  if(!settings.retroAchievements.username || !settings.retroAchievements.token || !httpWorker.start()) return false;
  HttpWorker::Task task;
  task.kind = HttpWorker::Task::Kind::Login;
  task.username = (const char*)settings.retroAchievements.username;
  task.token = (const char*)settings.retroAchievements.token;
  task.generation = ++_loginGeneration;
  _loginPending = true;
  _startupTokenLoginPending = startup;
  httpWorker.enqueue(std::move(task));
  return true;
}

auto RetroAchievements::login(const string& username, const string& password) -> bool {
  Program::Guard guard;
  if(!username || !password || !httpWorker.start()) return false;
  _startupTokenLoginPending = false;
  HttpWorker::Task task;
  task.kind = HttpWorker::Task::Kind::Login;
  task.username = (const char*)username;
  task.password = (const char*)password;
  task.generation = ++_loginGeneration;
  _loginPending = true;
  httpWorker.enqueue(std::move(task));
  return true;
}

auto RetroAchievements::logout() -> void {
  Program::Guard guard;
  ++_loginGeneration;
  _loginPending = false;
  _startupTokenLoginPending = false;
  startupLoginDialog.close(true);
  unloadClientGame();
  if(_client) rc_client_logout(_client);
  clearUser();
}

auto RetroAchievements::completeLogin(u64 generation, const RetroAchievementsLoginResult& result) -> void {
  Program::Guard guard;
  if(generation != _loginGeneration) return;
  bool startupTokenLogin = _startupTokenLoginPending;
  _startupTokenLoginPending = false;
  _loginPending = false;
  program.showMessage({"[RA] ", result.message});
  if(result.success) {
    setUser(result);
    settings.retroAchievements.enabled = true;
    settings.retroAchievements.username = result.username;
    settings.retroAchievements.token = result.token;
    settings.save();
    if(hardcore()) resetGame();
    syncClientLogin();
    startupLoginDialog.close();
  } else if(startupTokenLogin) {
    if(program.kiosk) program.showMessage("[RA] Sign in through the main GUI to enable achievements");
    else if(result.errorCode == RC_EXPIRED_TOKEN) startupLoginDialog.request("Saved RetroAchievements login expired. Please sign in again.");
    else if(result.errorCode == RC_INVALID_CREDENTIALS) startupLoginDialog.request("Saved RetroAchievements login is invalid. Please sign in again.");
    else startupLoginDialog.request({"Saved RetroAchievements login failed: ", result.message});
  } else if(startupLoginDialog.constructed && startupLoginDialog.window.visible()) {
    startupLoginDialog.setStatus(result.message, true);
    startupLoginDialog.setPending(false);
  }
  if(!result.success && (result.errorCode == RC_EXPIRED_TOKEN || result.errorCode == RC_INVALID_CREDENTIALS || result.errorCode == RC_LOGIN_REQUIRED)) {
    settings.retroAchievements.token = "";
    settings.save();
  }
  retroAchievementsSettings.statusLabel.setText(result.message);
  retroAchievementsSettings.refresh();
}

auto RetroAchievements::pumpUi() -> void {
  for(auto& response : httpWorker.takeLoginResponses()) {
    completeLogin(response.generation, response.result);
  }
  for(auto& response : httpWorker.takeAvatarResponses()) {
    retroAchievementsSettings.acceptAvatar(response.url, response.bytes);
  }
  startupLoginDialog.pump();
}

auto RetroAchievements::requestAvatar(const string& url) -> void {
  if(!url || !httpWorker.start()) return;
  HttpWorker::Task task;
  task.kind = HttpWorker::Task::Kind::Avatar;
  task.url = (const char*)url;
  httpWorker.enqueue(std::move(task));
}

auto RetroAchievements::hasUser() const -> bool {
  Program::Guard guard;
  return _authenticated;
}

auto RetroAchievements::loginPending() const -> bool {
  Program::Guard guard;
  return _loginPending;
}

auto RetroAchievements::username() const -> string {
  Program::Guard guard;
  return _username;
}

auto RetroAchievements::displayName() const -> string {
  Program::Guard guard;
  return _displayName ? _displayName : _username;
}

auto RetroAchievements::userScore() const -> u32 {
  Program::Guard guard;
  return _score;
}

auto RetroAchievements::userScoreSoftcore() const -> u32 {
  Program::Guard guard;
  return _scoreSoftcore;
}

auto RetroAchievements::avatarUrl() const -> string {
  Program::Guard guard;
  return _avatarUrl;
}

auto RetroAchievements::readMemory(u32 address, u8* buffer, u32 size) const -> u32 {
  if(!emulator) return 0;
  if(!settings.retroAchievements.enabled) return 0;
  if(auto* adapter = RA::Platform::selectAdapter(*emulator)) {
    return adapter->readMemory(*emulator, address, buffer, size);
  }
  return 0;
}

auto RetroAchievements::handleClientLoginResult(int result, const char* errorMessage, u64 generation) -> void {
  if(generation != _loginGeneration || !_authenticated) return;
  if(result != RC_OK) {
    _gameReady = false;
    program.showMessage({"[RA] Client login failed: ", errorMessage ? errorMessage : rc_error_str(result)});
    return;
  }

  if(auto user = _client ? rc_client_get_user_info(_client) : nullptr) {
    program.showMessage({"[RA] Client session ready for ", user->display_name ? user->display_name : user->username});
  }
  if(_gameHash) loadGame();
}

auto RetroAchievements::handleClientGameLoadResult(int result, const char* errorMessage, u64 generation) -> void {
  if(generation != _gameGeneration) return;
  _pendingRomData.clear();
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

  auto counts = visibleAchievementCounts(_client);
  program.showMessage({
    "[RA] Game loaded: ", title,
    " (", counts.unlocked, "/", counts.total,
    " achievements, ", subsetCount, " sets, ", hardcore() ? "Hardcore" : "Casual", ")"
  });
}

auto RetroAchievements::frame() -> void {
  httpWorker.pumpClient();
  if(!_client) return;
  if(!settings.retroAchievements.enabled) return;
  if(!_authenticated) return;
  if(_gameReady) rc_client_do_frame(_client);
  else rc_client_idle(_client);
}

auto RetroAchievements::idle() -> void {
  httpWorker.pumpClient();
  if(!_client) return;
  if(!settings.retroAchievements.enabled) return;
  if(!_authenticated) return;
  rc_client_idle(_client);
}

auto RetroAchievements::gameLoaded() -> void {
  Program::Guard guard;
  _gameHash = {};

  if(!emulator) return;
  if(!settings.retroAchievements.enabled) return;
  unloadClientGame();
  if(settings.retroAchievements.hardcore && emulator->game && emulator->game->pak && emulator->game->pak->attribute("label") == "Game Genie") {
    program.showMessage("[RA] Game Genie requires Casual mode");
  }
  if(RA::Platform::selectAdapter(*emulator)) {
    program.showMessage(hardcore() ? "[RA] Hardcore mode" : "[RA] Casual mode");
  }
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
}

auto RetroAchievements::gameUnloaded() -> void {
  Program::Guard guard;
  _gameHash = {};
  unloadClientGame();
}

auto RetroAchievements::gameHash() const -> string {
  Program::Guard guard;
  return _gameHash;
}

auto RetroAchievements::hardcore() const -> bool {
  return settings.retroAchievements.enabled && settings.retroAchievements.hardcore &&
    supportsGame() &&
    !(emulator->game && emulator->game->pak && emulator->game->pak->attribute("label") == "Game Genie");
}

auto RetroAchievements::supportsGame() const -> bool {
  return emulator && RA::Platform::selectAdapter(*emulator);
}

auto RetroAchievements::enableHardcore(bool enabled) -> bool {
  Program::Guard guard;
  if(enabled && supportsGame() && emulator->game && emulator->game->pak && emulator->game->pak->attribute("label") == "Game Genie") {
    program.showMessage("[RA] Hardcore is unavailable with a Game Genie cartridge");
    return false;
  }
  if(enabled == settings.retroAchievements.hardcore) return true;
  settings.retroAchievements.hardcore = enabled;
  bool clientGameLoaded = _client && rc_client_is_game_loaded(_client);
  if(_client) rc_client_set_hardcore_enabled(_client, hardcore() ? 1 : 0);
  if(enabled) {
    program.rewinding = false;
    program.requestFrameAdvance = false;
    program.rewindReset();
    program.runAheadUpdate();
    if(hardcore()) {
      nall::GDB::server.close();
      settings.developer.debugServerEnabled = false;
      developerSettings.enabled.setChecked(false);
      if(program.toolsWindowConstructed) toolsWindow.setVisible(false);
      if(!clientGameLoaded) resetGame();
    }
  } else {
    program.runAheadUpdate();
  }
  settings.save();
  program.showMessage(enabled ? "[RA] Hardcore enabled" : "[RA] Hardcore disabled");
  return true;
}

auto RetroAchievements::setEnabled(bool enabled) -> void {
  Program::Guard guard;
  if(settings.retroAchievements.enabled == enabled) return;
  settings.retroAchievements.enabled = enabled;
  if(!enabled) {
    ++_loginGeneration;
    _loginPending = false;
    _startupTokenLoginPending = false;
    startupLoginDialog.close(true);
    unloadClientGame();
  } else {
    if(emulator) {
      gameLoaded();
      if(hardcore()) resetGame();
    }
    if(!_authenticated && settings.retroAchievements.username && settings.retroAchievements.token) {
      if(!beginSavedTokenLogin(false)) retroAchievementsSettings.statusLabel.setText("RetroAchievements network initialization failed");
    }
  }
  settings.save();
  program.showMessage(enabled ? "[RA] Enabled" : "[RA] Disabled");
}

auto RetroAchievements::reset() -> void {
  if(_client && _gameReady) rc_client_reset(_client);
}

auto RetroAchievements::resetGame() -> void {
  Program::Guard guard;
  if(!emulator) return;
  program.rewindReset();
  emulator->root->power(true);
  reset();
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
