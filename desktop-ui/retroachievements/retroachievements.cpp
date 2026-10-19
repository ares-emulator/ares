#include "../desktop-ui.hpp"

#include "platform/adapter.hpp"
#include "platform/hash_chd.hpp"
#include "raintegration.hpp"
#if defined(_WIN32)
#define boolean RA_win_boolean
#endif
#include <curl/curl.h>
#include <rc_api_user.h>
#include <rc_client.h>
#include <rc_hash.h>
#if defined(_WIN32)
#undef boolean
#endif
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace {
struct ClientCallbackData {
  RetroAchievements* self = nullptr;
  u64 generation = 0;
};

struct LeaderboardCallbackData {
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
  char hash[33] = {};
  if(adapter->romData(emu, rom) && !rom.empty()) {
    if(rc_hash_generate_from_buffer(hash, adapter->consoleId(emu), rom.data(), rom.size())) return hash;
    return {};
  }

  auto path = adapter->filePath(emu);
  if(!path || !file::exists(path)) return {};

  rc_hash_iterator_t iterator{};
  rc_hash_initialize_iterator(&iterator, path, nullptr, 0);
  RA::Platform::configureChdHashCallbacks(iterator.callbacks);
  auto success = rc_hash_generate(hash, adapter->consoleId(emu), &iterator);
  rc_hash_destroy_iterator(&iterator);
  return success ? string{hash} : string{};
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

auto clientEvent(const rc_client_event_t* event, rc_client_t* client) -> void {
  if(!event) return;
  switch(event->type) {
  case RC_CLIENT_EVENT_ACHIEVEMENT_TRIGGERED:
    if(event->achievement && event->achievement->title) {
      const char* label = "Achievement unlocked: ";
      if(retroAchievements.encoreUnlock(event->achievement->id)) label = "Encore achievement unlocked: ";
      else if(event->achievement->category == RC_CLIENT_ACHIEVEMENT_CATEGORY_UNOFFICIAL) label = "Unofficial achievement unlocked: ";
      else if(retroAchievements.developmentMenuSupported()) label = RA::Integration::achievementUnlockLabel(client, event->achievement->id);
      program.showMessage({"[RA] ", label, event->achievement->title});
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

auto clientLeaderboardCallback(int result, const char* errorMessage, rc_client_leaderboard_entry_list_t* list, rc_client_t*, void* userdata) -> void {
  std::unique_ptr<LeaderboardCallbackData> data{static_cast<LeaderboardCallbackData*>(userdata)};
  if(data && data->self) data->self->handleLeaderboardResult(result, errorMessage, list, data->generation);
  if(list) rc_client_destroy_leaderboard_entry_list(list);
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
  explicit HttpWorker(bool manageCurl = true) : manageCurl(manageCurl) {}

  struct Task {
    enum class Kind { Client, Login, Image } kind;
    std::string url;
    std::string cachePath;
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
  struct ImageResponse {
    string url;
    std::vector<u8> bytes;
  };

  std::mutex mutex;
  std::condition_variable cv;
  std::deque<Task> tasks;
  std::deque<ClientResponse> clientResponses;
  std::deque<LoginResponse> loginResponses;
  std::deque<ImageResponse> imageResponses;
  std::thread thread;
  bool started = false;
  bool stopping = false;
  bool manageCurl = true;

  auto start() -> bool {
    if(started) return true;
    if(manageCurl && curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) return false;
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

  auto takeImageResponses() -> std::deque<ImageResponse> {
    std::lock_guard lock{mutex};
    std::deque<ImageResponse> out;
    out.swap(imageResponses);
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
    if(manageCurl) curl_global_cleanup();
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

      if(task.kind == Task::Kind::Image && !task.cachePath.empty()) {
        auto cached = file::read(task.cachePath.c_str());
        if(!cached.empty() && image{cached}) {
          std::lock_guard lock{mutex};
          imageResponses.push_back({task.url.c_str(), std::move(cached)});
          continue;
        }
        if(file::exists(task.cachePath.c_str())) file::remove(task.cachePath.c_str());
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

      if(task.kind == Task::Kind::Client) {
        std::lock_guard lock{mutex};
        clientResponses.push_back({std::move(body), status, task.callback, task.callbackData});
      } else {
        std::vector<u8> bytes;
        if(status >= 200 && status < 300) bytes.assign(body.begin(), body.end());
        if(!bytes.empty()) {
          if(image{bytes}) {
            auto path = string{task.cachePath.c_str()};
            directory::create(Location::path(path));
            auto temporaryPath = string{path, ".tmp"};
            if(file::write(temporaryPath, {bytes.data(), bytes.size()})) {
              if(!file::move(temporaryPath, path)) file::remove(temporaryPath);
            } else {
              file::remove(temporaryPath);
            }
          } else {
            bytes.clear();
          }
        }
        std::lock_guard lock{mutex};
        imageResponses.push_back({task.url.c_str(), std::move(bytes)});
      }
    }
  }
};

HttpWorker httpWorker;
HttpWorker imageWorker{false};

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
  _shuttingDown = true;
  unloadClientGame();
  stopIntegration();
  imageWorker.stop();
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
  rc_client_set_encore_mode_enabled(_client, settings.retroAchievements.encore ? 1 : 0);
  return true;
}

auto RetroAchievements::syncClientLogin() -> void {
  if(!_authenticated) return;
  if(!_username || !_token) return;
  if(!ensureClient()) return;

  if(RA::Integration::available() && !_integrationStarted) {
    _integrationStarted = true;
    _integrationPending = true;
    _integrationResumeLogin = true;
    _integrationLoadHandle = RA::Integration::start(*this, _client);
    return;
  }
  if(_integrationPending) return;

  if(auto user = rc_client_get_user_info(_client)) {
    if(user->username && _username == user->username) return;
  }

  rc_client_begin_login_with_token(_client, _username, _token, clientLoginCallback, new ClientCallbackData{this, _loginGeneration});
}

auto RetroAchievements::loadGame() -> void {
  _gameReady = false;
  _gameLoadPending = false;
  _gameLoadError = {};
  _gameUnrecognized = false;
  if(!settings.retroAchievements.enabled) return;
  if(!_authenticated) return;
  if(!emulator) return;
  if(!ensureClient()) return;
  rc_client_set_hardcore_enabled(_client, hardcore() ? 1 : 0);
  rc_client_set_encore_mode_enabled(_client, settings.retroAchievements.encore ? 1 : 0);

  auto* adapter = RA::Platform::selectAdapter(*emulator);
  if(!adapter) return;
  if(_integrationActive) RA::Integration::setConsoleId(_client, adapter->consoleId(*emulator));
  if(!_gameHash) return;
  _encoreGameActive = settings.retroAchievements.encore;
  _gameLoadPending = true;
  rc_client_begin_load_game(
    _client,
    _gameHash,
    clientLoadGameCallback,
    new ClientCallbackData{this, _gameGeneration}
  );
}

auto RetroAchievements::unloadClientGame() -> void {
  ++_gameGeneration;
  ++_leaderboardGeneration;
  if(_client && _leaderboardFetch) rc_client_abort_async(_client, _leaderboardFetch);
  _leaderboardFetch = nullptr;
  _leaderboardView = {};
  _gameReady = false;
  _gameLoadPending = false;
  _gameLoadError = {};
  _gameUnrecognized = false;
  _encoreGameActive = false;
  _reloadEncoreGame = false;
  _previousUnlocks.clear();
  if(_client) rc_client_unload_game(_client);
}

auto RetroAchievements::reloadEncoreGameIfNeeded() -> void {
  if(!_reloadEncoreGame) return;
  _reloadEncoreGame = false;
  unloadClientGame();
  loadGame();
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

auto RetroAchievements::logout() -> bool {
  Program::Guard guard;
  if(!confirmDevelopmentUnload()) return false;
  ++_loginGeneration;
  _loginPending = false;
  _startupTokenLoginPending = false;
  startupLoginDialog.close(true);
  unloadClientGame();
  if(_client) rc_client_logout(_client);
  clearUser();
  return true;
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
  if(program.settingsWindowConstructed) {
    retroAchievementsSettings.statusLabel.setText(result.message);
    retroAchievementsSettings.refresh();
  }
}

auto RetroAchievements::pumpUi() -> void {
  if(_integrationPending) {
    Program::Guard guard;
    httpWorker.pumpClient();
  }
  for(auto& response : httpWorker.takeLoginResponses()) {
    completeLogin(response.generation, response.result);
  }
  for(auto& response : imageWorker.takeImageResponses()) {
    auto key = std::string{(const char*)response.url};
    if(_pendingImages.erase(key)) {
      if(!response.bytes.empty()) {
        image picture{response.bytes};
        if(picture) {
          _images.emplace(std::move(key), std::move(picture));
          ++_imageVersion;
        } else _failedImages.insert(std::move(key));
      } else {
        _failedImages.insert(std::move(key));
      }
    }
    if(program.settingsWindowConstructed) retroAchievementsSettings.acceptAvatar(response.url, response.bytes);
  }
  startupLoginDialog.pump();
  if(_integrationResumeLogin && !_integrationPending) {
    _integrationResumeLogin = false;
    syncClientLogin();
  }
  if(_integrationHardcoreChanged.exchange(false)) {
    syncIntegrationHardcore();
  }
  if(_integrationPauseRequested.exchange(false)) {
    Program::Guard guard;
    program.pause(true);
  }
  if(!program.kiosk) presentation.refreshRetroAchievementsMenu();
}

auto RetroAchievements::requestImage(const string& url) -> void {
  if(_shuttingDown || !url || !httpWorker.start() || !imageWorker.start()) return;
  HttpWorker::Task task;
  task.kind = HttpWorker::Task::Kind::Image;
  task.url = (const char*)url;
  auto digest = Hash::SHA256({(const u8*)url.data(), url.size()}).digest();
  task.cachePath = (const char*)string{Path::userData(), "ares/cheevos/images/", digest, ".png"};
  imageWorker.enqueue(std::move(task));
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

auto RetroAchievements::imageForUrl(const string& url, u32 size) -> image {
  if(!url) return {};
  string absoluteUrl = url;
  if(absoluteUrl.beginsWith("/")) absoluteUrl = {"https://media.retroachievements.org", absoluteUrl};
  auto key = std::string{(const char*)absoluteUrl};
  if(auto it = _images.find(key); it != _images.end()) {
    image picture = it->second;
    if(picture && (picture.width() != size || picture.height() != size)) picture.scale(size, size);
    return picture;
  }
  if(_failedImages.count(key)) return {};
  if(_pendingImages.insert(key).second) requestImage(absoluteUrl);
  return {};
}

auto RetroAchievements::imageVersion() const -> u64 { return _imageVersion; }

auto RetroAchievements::gameView() const -> RetroAchievementsGameView {
  Program::Guard guard;
  RetroAchievementsGameView view;
  if(!settings.retroAchievements.enabled) {
    view.message = "RetroAchievements is disabled.";
    return view;
  }
  if(!_authenticated) {
    view.status = RetroAchievementsGameView::Status::LoginRequired;
    view.message = "Log in to RetroAchievements to view game achievements.";
    return view;
  }
  if(_gameLoadPending) {
    view.status = RetroAchievementsGameView::Status::Loading;
    view.message = "Loading RetroAchievements game data...";
    return view;
  }
  if(_gameUnrecognized) {
    view.status = RetroAchievementsGameView::Status::Unrecognized;
    view.message = "Game not recognized by RetroAchievements.";
    return view;
  }
  if(_gameLoadError) {
    view.status = RetroAchievementsGameView::Status::Error;
    view.message = _gameLoadError;
    return view;
  }
  if(!_gameReady || !_client) {
    view.status = RetroAchievementsGameView::Status::NoGame;
    view.message = "No RetroAchievements game session is active.";
    return view;
  }

  view.status = RetroAchievementsGameView::Status::Ready;
  if(auto* game = rc_client_get_game_info(_client); game && game->title) view.gameTitle = game->title;
  if(auto* list = rc_client_create_subset_list(_client)) {
    for(u32 index : range(list->num_subsets)) {
      auto* subset = list->subsets[index];
      if(!subset) continue;
      RetroAchievementsGameView::Set set;
      set.id = subset->id;
      set.title = subset->title ? subset->title : "Base Set";
      set.badgeUrl = subset->badge_url ? subset->badge_url : "";
      set.core = index == 0;
      view.sets.push_back(std::move(set));
    }
    rc_client_destroy_subset_list(list);
  }
  if(view.sets.empty()) {
    RetroAchievementsGameView::Set set;
    set.title = "Base Set";
    set.core = true;
    view.sets.push_back(std::move(set));
  }
  auto coreId = view.sets.front().id;
  if(auto* list = rc_client_create_achievement_list(_client, RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE_AND_UNOFFICIAL,
       RC_CLIENT_ACHIEVEMENT_LIST_GROUPING_LOCK_STATE)) {
    for(u32 bucketIndex : range(list->num_buckets)) {
      auto& bucket = list->buckets[bucketIndex];
      for(u32 index : range(bucket.num_achievements)) {
        auto* achievement = bucket.achievements[index];
        if(!achievement || achievementIsWarning(achievement)) continue;
        RetroAchievementsGameView::Achievement row;
        row.id = achievement->id;
        row.setId = bucket.subset_id ? bucket.subset_id : coreId;
        row.title = achievement->title ? achievement->title : "Untitled";
        row.description = achievement->description ? achievement->description : "";
        row.badgeUrl = achievement->badge_url ? achievement->badge_url : "";
        row.lockedBadgeUrl = achievement->badge_locked_url ? achievement->badge_locked_url : "";
        char badgeUrl[512] = {};
        if(!row.badgeUrl && rc_client_achievement_get_image_url(achievement, RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED,
             badgeUrl, sizeof(badgeUrl)) == RC_OK) row.badgeUrl = badgeUrl;
        if(!row.lockedBadgeUrl && rc_client_achievement_get_image_url(achievement, RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE,
             badgeUrl, sizeof(badgeUrl)) == RC_OK) row.lockedBadgeUrl = badgeUrl;
        row.progress = achievement->measured_progress[0] ? achievement->measured_progress : "";
        row.points = achievement->points;
        row.unlocked = achievementUnlocked(achievement);
        row.hardcoreUnlocked = achievement->unlocked & RC_CLIENT_ACHIEVEMENT_UNLOCKED_HARDCORE;
        row.unlockTime = row.unlocked ? (u64)achievement->unlock_time : 0;
        switch(achievement->type) {
        case RC_CLIENT_ACHIEVEMENT_TYPE_MISSABLE: row.type = "Missable"; break;
        case RC_CLIENT_ACHIEVEMENT_TYPE_PROGRESSION: row.type = "Progression"; break;
        case RC_CLIENT_ACHIEVEMENT_TYPE_WIN: row.type = "Win Condition"; break;
        }
        for(auto& set : view.sets) {
          if(set.id != row.setId) continue;
          ++set.total;
          set.pointsTotal += row.points;
          if(row.unlocked) {
            ++set.unlocked;
            set.pointsUnlocked += row.points;
          }
          break;
        }
        view.achievements.push_back(std::move(row));
      }
    }
    rc_client_destroy_achievement_list(list);
  }
  if(auto* list = rc_client_create_leaderboard_list(_client, RC_CLIENT_LEADERBOARD_LIST_GROUPING_NONE)) {
    for(u32 bucketIndex : range(list->num_buckets)) {
      auto& bucket = list->buckets[bucketIndex];
      for(u32 index : range(bucket.num_leaderboards)) {
        auto* leaderboard = bucket.leaderboards[index];
        if(!leaderboard) continue;
        RetroAchievementsGameView::Leaderboard row;
        row.id = leaderboard->id;
        row.setId = bucket.subset_id ? bucket.subset_id : coreId;
        row.title = leaderboard->title ? leaderboard->title : "Untitled";
        row.description = leaderboard->description ? leaderboard->description : "";
        row.trackerValue = leaderboard->tracker_value ? leaderboard->tracker_value : "";
        row.active = leaderboard->state == RC_CLIENT_LEADERBOARD_STATE_ACTIVE || leaderboard->state == RC_CLIENT_LEADERBOARD_STATE_TRACKING;
        row.lowerIsBetter = leaderboard->lower_is_better;
        view.leaderboards.push_back(std::move(row));
      }
    }
    rc_client_destroy_leaderboard_list(list);
  }
  std::stable_sort(view.leaderboards.begin(), view.leaderboards.end(), [](const auto& a, const auto& b) {
    if(a.active != b.active) return a.active > b.active;
    return a.title < b.title;
  });
  return view;
}

auto RetroAchievements::leaderboardView() const -> RetroAchievementsLeaderboardView {
  Program::Guard guard;
  return _leaderboardView;
}

auto RetroAchievements::fetchLeaderboard(u32 leaderboardId, bool topEntries) -> void {
  Program::Guard guard;
  if(!_client || !_gameReady || !leaderboardId) return;
  ++_leaderboardGeneration;
  if(_leaderboardFetch) rc_client_abort_async(_client, _leaderboardFetch);
  _leaderboardFetch = nullptr;
  _leaderboardView = {};
  _leaderboardView.leaderboardId = leaderboardId;
  _leaderboardView.topEntries = topEntries;
  _leaderboardView.pending = true;
  _leaderboardView.status = "Downloading leaderboard data...";
  auto* data = new LeaderboardCallbackData{this, _leaderboardGeneration};
  if(topEntries) {
    _leaderboardFetch = rc_client_begin_fetch_leaderboard_entries(_client, leaderboardId, 1, 100, clientLeaderboardCallback, data);
  } else {
    _leaderboardFetch = rc_client_begin_fetch_leaderboard_entries_around_user(_client, leaderboardId, 25, clientLeaderboardCallback, data);
  }
}

auto RetroAchievements::handleLeaderboardResult(int result, const char* errorMessage, rc_client_leaderboard_entry_list_t* list, u64 generation) -> void {
  if(generation != _leaderboardGeneration) return;
  _leaderboardFetch = nullptr;
  _leaderboardView.pending = false;
  if(result != RC_OK || !list) {
    _leaderboardView.status = {"Fetch failed: ", errorMessage ? errorMessage : rc_error_str(result)};
    return;
  }
  for(u32 index : range(list->num_entries)) {
    auto& entry = list->entries[index];
    RetroAchievementsLeaderboardView::Entry row;
    row.rank = entry.rank;
    row.user = entry.user ? entry.user : "Unknown";
    row.score = entry.display;
    row.isSelf = _username && row.user == _username;
    char url[512] = {};
    if(rc_client_leaderboard_entry_get_user_image_url(&entry, url, sizeof(url)) == RC_OK) row.avatarUrl = url;
    _leaderboardView.entries.push_back(std::move(row));
  }
  _leaderboardView.status = _leaderboardView.topEntries
    ? string{"Showing top ", list->num_entries, " of ", list->total_entries, " entries"}
    : list->user_index >= 0
      ? string{"Showing entries around your rank (", list->num_entries, " of ", list->total_entries, ")"}
      : string{"No submitted score found for your account. Showing top ", list->num_entries, " entries instead."};
}

auto RetroAchievements::readMemory(u32 address, u8* buffer, u32 size) const -> u32 {
  Program::Guard guard;
  if(!emulator) return 0;
  if(!settings.retroAchievements.enabled) return 0;
  if(auto* adapter = RA::Platform::selectAdapter(*emulator)) {
    return adapter->readMemory(*emulator, address, buffer, size);
  }
  return 0;
}

auto RetroAchievements::writeMemory(u32 address, const u8* buffer, u32 size) -> u32 {
  Program::Guard guard;
  if(!emulator || !settings.retroAchievements.enabled) return 0;
  if(auto* adapter = RA::Platform::selectAdapter(*emulator)) {
    return adapter->writeMemory(*emulator, address, buffer, size);
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
  _gameLoadPending = false;
  _gameReady = result == RC_OK && _client && rc_client_is_game_loaded(_client);

  if(result != RC_OK) {
    _gameReady = false;
    if(result == RC_NO_GAME_LOADED) {
      _gameUnrecognized = true;
      program.showMessage("[RA] Game not recognized by RetroAchievements");
    } else {
      _gameLoadError = {"Game load failed: ", errorMessage ? errorMessage : rc_error_str(result)};
      program.showMessage({"[RA] Game load failed: ", errorMessage ? errorMessage : rc_error_str(result)});
    }
    return;
  }

  _previousUnlocks.clear();
  if(_encoreGameActive) {
    if(auto* list = rc_client_create_achievement_list(_client, RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE_AND_UNOFFICIAL,
         RC_CLIENT_ACHIEVEMENT_LIST_GROUPING_LOCK_STATE)) {
      for(u32 bucketIndex : range(list->num_buckets)) {
        auto& bucket = list->buckets[bucketIndex];
        for(u32 index : range(bucket.num_achievements)) {
          auto* achievement = bucket.achievements[index];
          if(achievementUnlocked(achievement)) _previousUnlocks.insert(achievement->id);
        }
      }
      rc_client_destroy_achievement_list(list);
    }
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
    " achievements, ", subsetCount, " sets, ", hardcore() ? "Hardcore" : "Casual",
    _encoreGameActive ? ", Encore" : "", ")"
  });
}

auto RetroAchievements::frame() -> void {
  if(!_integrationPending) httpWorker.pumpClient();
  reloadEncoreGameIfNeeded();
  if(!_client) return;
  if(!settings.retroAchievements.enabled) return;
  if(!_authenticated) return;
  if(_gameReady) rc_client_do_frame(_client);
  else rc_client_idle(_client);
}

auto RetroAchievements::idle() -> void {
  if(!_integrationPending) httpWorker.pumpClient();
  reloadEncoreGameIfNeeded();
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

auto RetroAchievements::enableEncore(bool enabled) -> void {
  Program::Guard guard;
  if(settings.retroAchievements.encore == enabled) return;
  settings.retroAchievements.encore = enabled;
  settings.save();
  program.showMessage(enabled ? "[RA] Encore enabled for the next game load" : "[RA] Encore disabled for the next game load");
}

auto RetroAchievements::encoreUnlock(u32 achievementId) const -> bool {
  return _encoreGameActive && _previousUnlocks.contains(achievementId);
}

auto RetroAchievements::setEnabled(bool enabled) -> void {
  Program::Guard guard;
  if(settings.retroAchievements.enabled == enabled) return;
  if(!enabled && !confirmDevelopmentUnload()) return;
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
  if(!_client || !_gameReady) return;
  if(_encoreGameActive || _encoreGameActive != settings.retroAchievements.encore) {
    _reloadEncoreGame = true;
  } else {
    rc_client_reset(_client);
  }
}

auto RetroAchievements::resetGame() -> void {
  Program::Guard guard;
  if(!emulator) return;
  program.rewindReset();
  emulator->root->power(true);
  reset();
}

auto RetroAchievements::stopIntegration() -> void {
  RA::Integration::stop(_client, _integrationLoadHandle, _integrationActive);
  _integrationLoadHandle = nullptr;
  _integrationActive = false;
  _integrationPending = false;
}

auto RetroAchievements::syncIntegrationHardcore() -> void {
  Program::Guard guard;
  if(!_integrationActive) return;
  auto enabled = RA::Integration::hardcoreEnabled(_client);
  if(enabled == settings.retroAchievements.hardcore) return;
  settings.retroAchievements.hardcore = enabled;
  if(enabled) {
    program.rewinding = false;
    program.requestFrameAdvance = false;
    program.rewindReset();
    nall::GDB::server.close();
    settings.developer.debugServerEnabled = false;
    developerSettings.enabled.setChecked(false);
    if(program.toolsWindowConstructed) toolsWindow.setVisible(false);
  }
  program.runAheadUpdate();
  settings.save();
  if(program.settingsWindowConstructed) retroAchievementsSettings.refresh();
  program.showMessage(enabled ? "[RA] Hardcore enabled" : "[RA] Hardcore disabled");
}

auto RetroAchievements::handleIntegrationLoad(int result, const char* errorMessage) -> void {
  if(_shuttingDown) return;
  _integrationLoadHandle = nullptr;
  _integrationPending = false;
  _integrationActive = result == RC_OK;
  if(_integrationActive) {
    RA::Integration::finish(_client);
    ++_integrationMenuVersion;
    program.showMessage("[RA] RAIntegration loaded");
  } else if(result != RC_MISSING_VALUE) {
    program.showMessage({"[RA] RAIntegration unavailable: ", errorMessage ? errorMessage : rc_error_str(result)});
  }
}

auto RetroAchievements::handleIntegrationEvent(DevelopmentEvent event) -> void {
  if(_shuttingDown) return;
  switch(event) {
  case DevelopmentEvent::MenuChanged:
    ++_integrationMenuVersion;
    break;
  case DevelopmentEvent::HardcoreChanged:
    _integrationHardcoreChanged = true;
    ++_integrationMenuVersion;
    break;
  case DevelopmentEvent::Pause:
    _integrationPauseRequested = true;
    break;
  }
}

auto RetroAchievements::developmentMenuSupported() const -> bool {
  return settings.retroAchievements.enabled && _integrationActive;
}

auto RetroAchievements::developmentMenuVersion() const -> u64 {
  return _integrationMenuVersion.load();
}

auto RetroAchievements::developmentMenuItems() const -> std::vector<DevelopmentMenuItem> {
  Program::Guard guard;
  if(!developmentMenuSupported()) return {};
  return RA::Integration::menuItems(_client);
}

auto RetroAchievements::activateDevelopmentMenuItem(u32 id) -> bool {
  Program::Guard guard;
  if(!developmentMenuSupported()) return false;
  auto result = RA::Integration::activateMenuItem(_client, id);
  if(result) ++_integrationMenuVersion;
  return result;
}

auto RetroAchievements::confirmDevelopmentUnload() const -> bool {
  Program::Guard guard;
  if(!_integrationActive || !RA::Integration::hasModifications(_client)) return true;
  if(program.kiosk) {
    program.showMessage("[RA] Unsaved achievement changes will be lost when the game is unloaded");
    return true;
  }
  return MessageDialog().setTitle("RetroAchievements").setText(
    "There are unsaved achievement changes. Discard them and unload the game?"
  ).setAlignment(presentation).question({"Discard Changes", "Cancel"}) == "Discard Changes";
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
