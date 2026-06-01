#include "../desktop-ui.hpp"

#if defined(ARES_ENABLE_RCHEEVOS)
  #include <curl/curl.h>
  #include <rc_api_user.h>
  #include <rc_consoles.h>
  #include <rc_hash.h>
#endif

RetroAchievements retroAchievements;

#if defined(ARES_ENABLE_RCHEEVOS)
namespace {
auto appendPakFile(const std::shared_ptr<vfs::directory>& pak, const string& name, std::vector<u8>& out) -> bool {
  if(!pak) return false;
  auto fp = pak->read(name);
  if(!fp) return false;
  auto offset = out.size();
  out.resize(offset + fp->size());
  for(u32 index : range(fp->size())) out[offset + index] = fp->read();
  return true;
}

auto consoleId(const Emulator& emu) -> u32 {
  if(emu.name == "Famicom") return RC_CONSOLE_NINTENDO;
  if(emu.name == "Mega Drive") return RC_CONSOLE_MEGA_DRIVE;
  return RC_CONSOLE_UNKNOWN;
}

auto romData(const Emulator& emu, std::vector<u8>& out) -> bool {
  out.clear();
  if(!emu.game || !emu.game->pak) return false;

  if(emu.name == "Famicom") {
    auto hasInes = appendPakFile(emu.game->pak, "ines.rom", out);
    auto hasProgramFlash = appendPakFile(emu.game->pak, "program.flash", out);
    auto hasProgram = appendPakFile(emu.game->pak, "program.rom", out);
    auto hasOption = appendPakFile(emu.game->pak, "option.rom", out);
    auto hasCharacter = appendPakFile(emu.game->pak, "character.rom", out);
    return hasInes || hasProgramFlash || hasProgram || hasOption || hasCharacter;
  }

  if(emu.name == "Mega Drive") {
    return appendPakFile(emu.game->pak, "program.rom", out);
  }

  return false;
}

auto generateHash(const Emulator& emu) -> string {
  auto id = consoleId(emu);
  if(id == RC_CONSOLE_UNKNOWN) return {};

  std::vector<u8> rom;
  if(!romData(emu, rom) || rom.empty()) return {};

  char hash[33] = {};
  if(!rc_hash_generate_from_buffer(hash, id, rom.data(), rom.size())) return {};
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
    settings.retroAchievements.username = result.username;
    settings.retroAchievements.token = result.token;
    settings.save();
  }
  program.showMessage({"[RA] ", result.message});
#endif
}

auto RetroAchievements::login(const string& username, const string& password) -> RetroAchievementsLoginResult {
#if defined(ARES_ENABLE_RCHEEVOS)
  return loginRequest(username, password, "");
#else
  RetroAchievementsLoginResult result;
  result.message = "Rebuild with ARES_ENABLE_RCHEEVOS to use RetroAchievements";
  return result;
#endif
}

auto RetroAchievements::gameLoaded() -> void {
  _gameHash = {};

#if defined(ARES_ENABLE_RCHEEVOS)
  if(!emulator) return;
  if(!settings.retroAchievements.enabled) return;

  auto hash = generateHash(*emulator);
  if(!hash) {
    if(consoleId(*emulator) != RC_CONSOLE_UNKNOWN) {
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
