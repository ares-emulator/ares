struct RetroAchievementsLoginResult {
  bool success = false;
  string username;
  string token;
  string displayName;
  string avatarUrl;
  u32 score = 0;
  u32 scoreSoftcore = 0;
  string message;
};

#if defined(ARES_ENABLE_RCHEEVOS)
struct rc_client_t;
#endif

struct RetroAchievements {
  ~RetroAchievements();

  auto initialize() -> void;
  auto login(const string& username, const string& password) -> RetroAchievementsLoginResult;
  auto logout() -> void;
  auto hasUser() const -> bool;
  auto username() const -> string;
  auto displayName() const -> string;
  auto userScore() const -> u32;
  auto userScoreSoftcore() const -> u32;
  auto avatarUrl() const -> string;
  auto readMemory(u32 address, u8* buffer, u32 size) const -> u32;
  auto handleClientLoginResult(int result, const char* errorMessage) -> void;
  auto handleClientGameLoadResult(int result, const char* errorMessage) -> void;
  auto gameLoaded() -> void;
  auto gameUnloaded() -> void;
  auto gameHash() const -> string;

private:
  auto ensureClient() -> bool;
  auto syncClientLogin() -> void;
  auto loadGame() -> void;
  auto unloadClientGame() -> void;
  auto setUser(const RetroAchievementsLoginResult& result) -> void;
  auto clearUser() -> void;

#if defined(ARES_ENABLE_RCHEEVOS)
  rc_client_t* _client = nullptr;
#endif
  string _gameHash;
  bool _gameReady = false;
  bool _gameLoadPending = false;
  string _username;
  string _token;
  string _displayName;
  string _avatarUrl;
  u32 _score = 0;
  u32 _scoreSoftcore = 0;
  bool _authenticated = false;
};

extern RetroAchievements retroAchievements;
